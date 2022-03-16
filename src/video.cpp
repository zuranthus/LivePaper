#include "video.h"
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
    StreamDecoderContext ctx;

    AVFormatContext* avformat_ctx{};
    auto err = avformat_open_input(&avformat_ctx, std::bit_cast<const char*>(path.u8string().c_str()), NULL, NULL);
    if (err < 0) return MakeError(avstrerr(err));
    ctx.avformat = make_raii_ptr(avformat_ctx, avformat_close_input);

    err = avformat_find_stream_info(ctx.avformat.get(), NULL);
    if (err < 0) return MakeError(avstrerr(err));

    AVCodec* codec{};
    auto stream_id = av_find_best_stream(ctx.avformat.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream_id < 0) return MakeError(avstrerr(stream_id));
    ctx.stream_id = stream_id;
    auto stream = ctx.avformat->streams[stream_id];

    auto avcodec_ctx = avcodec_alloc_context3(codec);
    if (!avcodec_ctx) return MakeError("Failed to allocate AVCodecContext");
    ctx.avcodec = make_raii_ptr(avcodec_ctx, avcodec_free_context);

    err = avcodec_parameters_to_context(ctx.avcodec.get(), stream->codecpar);
    if (err < 0) return MakeError(avstrerr(err));
    err = avcodec_open2(ctx.avcodec.get(), codec, NULL);
    if (err < 0) return MakeError(avstrerr(err));

    return VideoDecoder(std::move(ctx));
}

VideoDecoder::VideoDecoder(StreamDecoderContext&& context) 
: context(std::move(context))
, packet(make_raii_ptr(av_packet_alloc(), av_packet_free))
, frame(make_raii_ptr(av_frame_alloc(), av_frame_free))
{}

auto VideoDecoder::NextFrame() -> expected<AVFrame*> {
    if (!HasFrames()) return make_unexpected_str("No frames left");

    for (;;) {
        // pkt->pts, pkt->dts and pkt->duration are always set to correct values in AVStream.time_base units
        // (and guessed if the format cannot provide them). pkt->pts can be AV_NOPTS_VALUE if the video format
        // has B-frames, so it is better to rely on pkt->dts if you do not decompress the payload.
        auto ret = av_read_frame(context.avformat.get(), packet.get());
        if (ret < 0 && ret != AVERROR_EOF) return make_unexpected_str(avstrerr(ret));

        // make sure unref will be called on packet
        std::unique_ptr<AVPacket, void(*)(AVPacket*)> packet_unref_guard {packet.get(), av_packet_unref};
        
        if (ret == AVERROR_EOF) { // reached the end of file
            finished = true;
            return make_unexpected_str("No frames left");
        }

        if (packet->stream_index == context.stream_id) {
            // for video streams 1 packet == 1 frame
            int ret = avcodec_send_packet(context.avcodec.get(), packet.get());
            if (ret < 0) return make_unexpected_str(avstrerr(ret));
            ret = avcodec_receive_frame(context.avcodec.get(), frame.get());
            if (ret < 0 && ret != AVERROR(EAGAIN)) return make_unexpected_str(avstrerr(ret));
            if (ret == AVERROR(EAGAIN)) {
                continue; // read frame again
            }

            return frame.get();
        }
    }
    assert(false);
    return make_unexpected_str("oops");
}

auto VideoDecoder::Reset() -> expected<void> {
    finished = false;
    avcodec_flush_buffers(context.avcodec.get());
    auto ret = av_seek_frame(context.avformat.get(), context.stream_id, 0, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return make_unexpected_str(avstrerr(ret));
    return {};
}

}
