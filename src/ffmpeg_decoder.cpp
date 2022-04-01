#include "ffmpeg_decoder.h"
#include <assert.h>

namespace ffmpeg_decoder {

namespace {
template <typename T, typename D = std::function<void(T*)>>
auto make_raii_ptr(T* res, void (*del)(T**)) -> std::unique_ptr<T, D> {
    return {res, [del](T* p) { del(&p); }};
}

auto avstrerr(int errnum) {
    char errbuf[1024];
    int res = av_strerror(errnum, errbuf, 1024);
    assert(res == 0);
    return std::string(errbuf);
}

auto unexpected_averror(int errnum) {
    return tl::unexpected(avstrerr(errnum));
}

// Returned unique_ptr calls av_packet_unref on avpacket when destroyed
struct AVPacketInfo {
    using auto_unref = std::unique_ptr<AVPacket, void(*)(AVPacket*)>;
    auto_unref packet_unref;
    chrono_ms start_time{0ms};
    chrono_ms end_time{0ms};
};
auto NextAVPacket(AVFormatContext* avformat, AVStream* avstream, AVPacket* avpacket)
        -> tl::expected<AVPacketInfo, int> {
    auto ret = av_read_frame(avformat, avpacket);
    if (ret < 0) return tl::unexpected(ret);
    // make sure unref will be called on packet
    AVPacketInfo::auto_unref packet_unref{avpacket, av_packet_unref};
    if (avpacket->stream_index != avstream->index) return tl::unexpected(AVERROR(EAGAIN));
    // TODO replace pts with dts? Per avformat doc:
    // pkt->pts can be AV_NOPTS_VALUE if the video format has B-frames,
    // so it is better to rely on pkt->dts if you do not decompress the payload.
    auto chrono_ms_k = av_q2d(avstream->time_base)*1000;
    chrono_ms start_time(static_cast<chrono_ms::rep>(avpacket->pts*chrono_ms_k));
    chrono_ms end_time(static_cast<chrono_ms::rep>((avpacket->pts + avpacket->duration)*chrono_ms_k));
    return AVPacketInfo{ std::move(packet_unref), start_time, end_time };
}

auto ResetStreamDecoding(AVFormatContext* avformat, AVCodecContext* avcodec, AVStream* avstream) 
        -> tl::expected<void, int> {
    avcodec_flush_buffers(avcodec);
    auto ret = av_seek_frame(avformat, avstream->index, 0, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return tl::unexpected(ret);
    return {};
}

auto CalculateStreamDuration(AVFormatContext* avformat, AVStream* avstream, AVPacket* avpacket) -> tl::expected<chrono_ms, int> {
    auto duration = 0ms;
    while (true) {
        auto packet_info = NextAVPacket(avformat, avstream, avpacket);
        if (!packet_info) {
            auto error = packet_info.error();
            if (error == AVERROR(EAGAIN)) continue; // current packet belongs to another stream
            if (error == AVERROR_EOF) break; // end of stream
            return tl::unexpected(error);
        }
        duration = std::max(duration, packet_info->end_time);
    }
    return duration;
}

}

auto FileLoader::MakeError(std::string_view message) const {
    return make_unexpected_str("{}\nFile: '{}'", message, path.string());
}

auto FileLoader::VideoStreamDecoder() -> expected<VideoDecoder> {
    AVFormatContext* avformat_ctx{};
    auto err = avformat_open_input(&avformat_ctx, std::bit_cast<const char*>(path.u8string().c_str()), NULL, NULL);
    if (err < 0) return MakeError(avstrerr(err));
    auto avformat_ptr = make_raii_ptr(avformat_ctx, avformat_close_input);

    err = avformat_find_stream_info(avformat_ctx, NULL);
    if (err < 0) return MakeError(avstrerr(err));

    AVCodec* codec{};
    auto stream_id = av_find_best_stream(avformat_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream_id < 0) return MakeError(avstrerr(stream_id));

    auto avcodec_ctx = avcodec_alloc_context3(codec);
    if (!avcodec_ctx) return MakeError("Failed to allocate AVCodecContext");
    auto avcodec_ptr = make_raii_ptr(avcodec_ctx, avcodec_free_context);

    auto avstream = avformat_ctx->streams[stream_id];
    err = avcodec_parameters_to_context(avcodec_ctx, avstream->codecpar);
    if (err < 0) return MakeError(avstrerr(err));
    err = avcodec_open2(avcodec_ctx, codec, NULL);
    if (err < 0) return MakeError(avstrerr(err));

    auto avpacket = make_raii_ptr(av_packet_alloc(), av_packet_free);
    auto duration = CalculateStreamDuration(avformat_ctx, avstream, avpacket.get());
    if (!duration) return unexpected_averror(duration.error());
    auto reset_success = ResetStreamDecoding(avformat_ctx, avcodec_ctx, avstream);
    if (!reset_success) return unexpected_averror(duration.error());
    return VideoDecoder({ std::move(avformat_ptr), std::move(avcodec_ptr), stream_id, duration.value() });
}

VideoDecoder::VideoDecoder(StreamDecoderContext&& context) 
: context(std::move(context))
, avpacket(make_raii_ptr(av_packet_alloc(), av_packet_free))
, avframe(make_raii_ptr(av_frame_alloc(), av_frame_free))
{}

auto VideoDecoder::NextFrame() -> expected<Frame> {
    if (!HasFrames()) return make_unexpected_str("No frames left");

    for (;;) {
        auto packet_info = NextAVPacket(context.avformat(), context.avstream(), avpacket.get());
        if (!packet_info) {
            auto error = packet_info.error();
            if (error == AVERROR(EAGAIN)) continue; // current packet belongs to another stream
            if (error == AVERROR_EOF) {
                // end of stream
                finished = true;
                return make_unexpected_str("No frames left");
            }
            return unexpected_averror(error);
        }

        auto avret = avcodec_send_packet(context.avcodec(), avpacket.get());
        if (avret < 0) return unexpected_averror(avret);
        avret = avcodec_receive_frame(context.avcodec(), avframe.get());
        if (avret == AVERROR(EAGAIN)) continue;
        if (avret < 0) return unexpected_averror(avret);

        return Frame{
            make_raii_ptr(av_frame_clone(avframe.get()), av_frame_free),
            packet_info->start_time,
            packet_info->end_time
        };
    }
    assert(false);
    return make_unexpected_str("Internal error");
}

auto VideoDecoder::Reset() -> expected<void> {
    auto ret = ResetStreamDecoding(context.avformat(), context.avcodec(), context.avstream());
    if (!ret) return unexpected_averror(ret.error());
    finished = false;
    return {};
}
}
