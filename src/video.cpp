#include "video.h"
#include <assert.h>

namespace {

auto avstrerr(int errnum) {
    char errbuf[1024];
    int res = av_strerror(errnum, errbuf, 1024);
    assert(res == 0);
    return std::string(errbuf);
}

auto make_error(std::string_view error, const std::filesystem::path& path) {
    return make_unexpected_str("Error: {}\nFile: '{}'", error, path.string());
}

template <typename T, typename D = std::function<void(T*)>>
auto make_raii_ptr(T* res, void (*del)(T**)) -> std::unique_ptr<T, D> {
    return {res, [del](T* p) { del(&p); }};
}

}

auto VideoStreamDecoder::FromFile(const std::filesystem::path& path) -> expected<VideoStreamDecoder> {
    VideoStreamDecoder::Context ctx;

    AVFormatContext* avformat_ctx{};
    int err = avformat_open_input(&avformat_ctx, std::bit_cast<const char*>(path.u8string().c_str()), NULL, NULL);
    if (err < 0) return make_error(avstrerr(err), path);
    ctx.avformat = make_raii_ptr(avformat_ctx, avformat_close_input);

    err = avformat_find_stream_info(ctx.avformat.get(), NULL);
    if (err < 0) return make_error(avstrerr(err), path);

    AVCodec* codec{};
    auto stream_id = av_find_best_stream(ctx.avformat.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream_id < 0) return make_error(avstrerr(stream_id), path);
    ctx.stream_id = stream_id;
    auto stream = ctx.avformat->streams[stream_id];

    auto avcodec_ctx = avcodec_alloc_context3(codec);
    if (!avcodec_ctx) return make_error("Failed to allocate AVCodecContext", path);
    ctx.avcodec = make_raii_ptr(avcodec_ctx, avcodec_free_context);

    err = avcodec_parameters_to_context(ctx.avcodec.get(), stream->codecpar);
    if (err < 0) return make_error(avstrerr(err), path);
    err = avcodec_open2(ctx.avcodec.get(), codec, NULL);
    if (err < 0) return make_error(avstrerr(err), path);

    return VideoStreamDecoder(std::move(ctx));
}

auto DecodeNextFrame() {
    // Decode -> Frame {AVFrame, time?, frame num?, ...}
    /*
        for (;;) {
    AVPacket packet;
    int ret = av_read_frame(v->input_ctx, &packet);
    if (ret < 0 && ret != AVERROR_EOF) FAIL_WITH("%s", av_err2str(ret));
    if (ret == AVERROR_EOF) { // reached the end of file
        av_packet_unref(&packet);
        return false;
    }

    if (packet.stream_index == v->stream_id) {
        // for video streams 1 packet == 1 frame
        if (avcodec_send_packet(v->decoder_ctx, &packet) < 0) FAIL();
        int ret = avcodec_receive_frame(v->decoder_ctx, frame);
        if (ret < 0 && ret != AVERROR(EAGAIN)) FAIL_WITH("%s", av_err2str(ret));
        if (ret == AVERROR(EAGAIN)) {
            av_frame_unref(frame);
            av_packet_unref(&packet);
            continue; // read frame again
        }

        *frame_end_time = (frame->best_effort_timestamp + frame->pkt_duration)*v->time_base;
        return true;
    }
    av_packet_unref(&packet);
    }*/
}

auto Restart() {
    // Seek to 0
   // avcodec_flush_buffers(v->decoder_ctx);
   // av_seek_frame(v->input_ctx, v->stream_id, 0, AVSEEK_FLAG_BACKWARD);
}