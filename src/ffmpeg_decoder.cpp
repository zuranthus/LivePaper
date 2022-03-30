#include "ffmpeg_decoder.h"
#include <assert.h>

namespace ffmpeg_decoder {

namespace {
auto avstrerr(int errnum) {
    char errbuf[1024];
    int res = av_strerror(errnum, errbuf, 1024);
    assert(res == 0);
    return std::string(errbuf);
}

template <typename T, typename D = std::function<void(T*)>>
auto make_raii_ptr(T* res, void (*del)(T**)) -> std::unique_ptr<T, D> {
    return {res, [del](T* p) { del(&p); }};
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

    err = avcodec_parameters_to_context(avcodec_ctx, avformat_ctx->streams[stream_id]->codecpar);
    if (err < 0) return MakeError(avstrerr(err));
    err = avcodec_open2(avcodec_ctx, codec, NULL);
    if (err < 0) return MakeError(avstrerr(err));

    return VideoDecoder({
        std::move(avformat_ptr), std::move(avcodec_ptr), stream_id
    });
}

VideoDecoder::VideoDecoder(StreamDecoderContext&& context) 
: context(std::move(context))
, avpacket(make_raii_ptr(av_packet_alloc(), av_packet_free))
, avframe(make_raii_ptr(av_frame_alloc(), av_frame_free))
{}

auto VideoDecoder::NextFrame() -> expected<Frame> {
    if (!HasFrames()) return make_unexpected_str("No frames left");

    for (;;) {
        // pkt->pts, pkt->dts and pkt->duration are always set to correct values in AVStream.time_base units
        // (and guessed if the format cannot provide them). pkt->pts can be AV_NOPTS_VALUE if the video format
        // has B-frames, so it is better to rely on pkt->dts if you do not decompress the payload.
        auto ret = av_read_frame(context.avformat(), avpacket.get());
        if (ret < 0 && ret != AVERROR_EOF) return make_unexpected_str(avstrerr(ret));

        // make sure unref will be called on packet
        std::unique_ptr<AVPacket, void(*)(AVPacket*)> packet_unref_guard {avpacket.get(), av_packet_unref};
        
        if (ret == AVERROR_EOF) { // reached the end of file
            finished = true;
            return make_unexpected_str("No frames left");
        }

        if (avpacket->stream_index == context.avstream()->index) {
            // for video streams 1 packet == 1 frame
            int ret = avcodec_send_packet(context.avcodec(), avpacket.get());
            if (ret < 0) return make_unexpected_str(avstrerr(ret));
            ret = avcodec_receive_frame(context.avcodec(), avframe.get());
            if (ret < 0 && ret != AVERROR(EAGAIN)) return make_unexpected_str(avstrerr(ret));
            if (ret == AVERROR(EAGAIN)) {
                continue; // read frame again
            }

            auto chrono_ms_k = av_q2d(context.avstream()->time_base)*1000;
            chrono_ms start_time(static_cast<long long>(avframe->best_effort_timestamp*chrono_ms_k));
            chrono_ms end_time(static_cast<long long>((
                avframe->best_effort_timestamp + avframe->pkt_duration)*chrono_ms_k));
            return Frame{
                avframe.get(),
                start_time,
                end_time
            };
        }
    }
    assert(false);
    return make_unexpected_str("oops");
}

auto VideoDecoder::Reset() -> expected<void> {
    finished = false;
    avcodec_flush_buffers(context.avcodec());
    auto ret = av_seek_frame(context.avformat(), context.avstream()->index, 0, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return make_unexpected_str(avstrerr(ret));
    return {};
}

}