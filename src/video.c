#include "video.h"

#include <assert.h>
#include <stdbool.h>

#include <SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>

#include "fail.h"
#include "platform.h"

struct CacheEntry {
    SDL_Texture *tex;
    double frame_end_time;
};

struct Video {
    AVCodecContext *decoder_ctx;
    AVFormatContext *input_ctx;
    struct SwsContext *sws_ctx;
    AVFrame *sws_frame;
    int stream_id;
    double time_base;

    double time;
    double next_frame_time;

    SDL_Texture *tex;
    SDL_Rect tex_dest;
};

static bool DecodeNextFrame(AVFrame *frame, double *frame_end_time, struct Video *v) {
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

            *frame_end_time = frame->best_effort_timestamp*v->time_base + frame->duration*av_q2d(frame->time_base);
            return true;
        }
        av_packet_unref(&packet);
    }
}

struct Video *VideoLoad(const struct Context *ctx) {
    struct Video *v = av_mallocz(sizeof(struct Video));

    if (avformat_open_input(&v->input_ctx, ctx->file, NULL, NULL) != 0)
        FAIL_WITH("Can't open file '%s'", ctx->file);
    if (avformat_find_stream_info(v->input_ctx, NULL) < 0)
        FAIL_WITH("Can't find stream information");
    const AVCodec *codec = NULL;
    v->stream_id = av_find_best_stream(v->input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (v->stream_id < 0) FAIL();

    v->decoder_ctx = avcodec_alloc_context3(codec);
    AVStream *stream = v->input_ctx->streams[v->stream_id];
    avcodec_parameters_to_context(v->decoder_ctx, stream->codecpar);
    if (avcodec_open2(v->decoder_ctx, codec, NULL) < 0)
        FAIL();
    v->time_base = av_q2d(stream->time_base);

    int w = v->decoder_ctx->width;
    int h = v->decoder_ctx->height;
    enum AVPixelFormat target_fmt = AV_PIX_FMT_RGB24;
    v->sws_ctx = sws_getContext( // convert to RGB24, no resize
        w, h, v->decoder_ctx->pix_fmt,
        w, h, target_fmt,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL);
    if (v->sws_ctx == NULL)
        FAIL();
    v->sws_frame = av_frame_alloc();
    v->sws_frame->format = target_fmt;
    v->sws_frame->width = w;
    v->sws_frame->height = h;
    if (av_frame_get_buffer(v->sws_frame, 32))
        FAIL();

    int win_w, win_h;
    SDL_GetWindowSize(ctx->window, &win_w, &win_h);
    float aspect_ratio = 1.0;
    if (ctx->fit == FIT_FIT) aspect_ratio = fminf((float)win_w/w, (float)win_h/h);
    else if (ctx->fit == FIT_FILL) aspect_ratio = fmaxf((float)win_w/w, (float)win_h/h);
    const int target_w = (int)(w * aspect_ratio);
    const int target_h = (int)(h * aspect_ratio);
    const SDL_Rect dest_rect = {
        .x = (win_w - target_w)/2,
        .y = (win_h - target_h)/2,
        .w = target_w,
        .h = target_h
    };
    v->tex_dest = dest_rect;
    v->tex = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!v->tex) FAIL_WITH("can't create SDL texture %i x %i pix", w, h);

    return v;
}

static void ResetVideo(struct Video *v) {
    avcodec_flush_buffers(v->decoder_ctx);
    av_seek_frame(v->input_ctx, v->stream_id, 0, AVSEEK_FLAG_BACKWARD);
}

void VideoUpdate(double delta_sec, struct Video *v, const struct Context *ctx) {
    assert(v);
    assert(v->input_ctx);

    v->time += delta_sec;
    if (v->time < v->next_frame_time) {  // previously rendered frame is still valid, nothing to do here
        return;
    }

    for (bool found_frame = false; !found_frame;) {
        AVFrame frame = {0};
        double frame_end_time = 0.0;
        if (!DecodeNextFrame(&frame, &frame_end_time, v)) {
            // no frames left (EOF), restart the video
            ResetVideo(v);
            v->time = 0.0;
            v->next_frame_time = -1.0;
            continue;
        }
        if (frame_end_time < v->next_frame_time) FAIL();  // something wrong with frame order
        if (frame_end_time > v->time) {
            found_frame = true;
            v->next_frame_time = frame_end_time;

            sws_scale(v->sws_ctx, (uint8_t const *const *)frame.data, frame.linesize, 0, v->decoder_ctx->height,
                      v->sws_frame->data, v->sws_frame->linesize);

            void *pix;
            int pitch;
            SDL_LockTexture(v->tex, NULL, &pix, &pitch);
            for (int y = 0; y < v->sws_frame->height; ++y)
                memcpy(pix + y * pitch, v->sws_frame->data[0] + y * v->sws_frame->linesize[0], v->sws_frame->width * 3);
            SDL_UnlockTexture(v->tex);
        }

        av_frame_unref(&frame);
    }

    SDL_SetRenderDrawColor(ctx->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, v->tex, NULL, &v->tex_dest);
    SDL_RenderPresent(ctx->renderer);
}

void VideoClear(struct Video *v, const struct Context *ctx) {
    if (v->tex) SDL_DestroyTexture(v->tex);
    av_frame_free(&v->sws_frame);
    sws_freeContext(v->sws_ctx);
    avcodec_free_context(&v->decoder_ctx);
    avformat_close_input(&v->input_ctx);
    memset(v, 0, sizeof(*v));
    av_free(v);
}
