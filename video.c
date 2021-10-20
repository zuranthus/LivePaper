#include "fail.h"
#include <assert.h>
#include <stdbool.h>

#include <SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "platform.h"

struct Video {
    AVCodecContext *decoder_ctx;
    AVFormatContext *input_ctx;
    struct SwsContext *sws_ctx;
    int video_stream;
    double total_time;
    double video_time_base;

    double current_time;
    double current_frame_end_time;

    AVFrame *frame;
    SDL_Texture *tex;
    SDL_Rect tex_dest;
};

struct Video* VideoLoad(char *path, const Context *context) {
    struct Video *v = av_mallocz(sizeof(struct Video));

    if (avformat_open_input(&v->input_ctx, path, NULL, NULL)!=0)
        FAIL_WITH("Couldn't open file");
    if (avformat_find_stream_info(v->input_ctx, NULL)<0)
        FAIL_WITH("Couldn't find stream information");

    AVCodec *codec = NULL;
    v->video_stream = av_find_best_stream(v->input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (v->video_stream < 0) FAIL();

    v->decoder_ctx = avcodec_alloc_context3(codec);
    assert(v->decoder_ctx);
    AVStream *stream = v->input_ctx->streams[v->video_stream];
    avcodec_parameters_to_context(v->decoder_ctx, stream->codecpar);
    if(avcodec_open2(v->decoder_ctx, codec, NULL)<0)
        FAIL();
    v->video_time_base = av_q2d(stream->time_base);
    v->total_time = stream->duration*v->video_time_base;

    v->frame = av_frame_alloc();

    int w = v->decoder_ctx->width;
    int h = v->decoder_ctx->height;
    v->sws_ctx = sws_getContext(
        w, h, v->decoder_ctx->pix_fmt,
        w, h, AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    v->tex = SDL_CreateTexture(context->renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!v->tex) FAIL_WITH("can't create SDL texture %i x %i pix", w, h);

    int win_w, win_h;
    SDL_GetWindowSize(context->window, &win_w, &win_h);
    const float aspect_ratio = fmaxf((float)win_w/w, (float)win_h/h);
    const int target_w = (int)(w * aspect_ratio);
    const int target_h = (int)(h * aspect_ratio);
    const SDL_Rect dest_rect = {
        .x = (win_w - target_w)/2,
        .y = (win_h - target_h)/2,
        .w = target_w,
        .h = target_h
    };
    v->tex_dest = dest_rect;

    return v;
}

bool VideoNextFrame(double delta_sec, struct Video *v, const Context *context) {
    assert(v);
    assert(v->input_ctx);

    v->current_time += delta_sec;

    printf("\nUPDATE %.3f/%.2f\n", v->current_time, v->current_frame_end_time);
    if (v->current_time < v->current_frame_end_time) {
        printf("skipping\n");
        return false;
    }

    bool done = false;
    bool found_frame = false;

    for(;;) {
        AVPacket packet;
        int ret = av_read_frame(v->input_ctx, &packet);
        if (ret == AVERROR_EOF) {
            printf("EOF ");
            if (v->current_time < v->total_time) {       
                done = true;
                printf("waiting for end of video\n");
            } else {
                v->current_time = fmod(v->current_time, v->total_time);
                v->current_frame_end_time = -1.0;
                avcodec_flush_buffers(v->decoder_ctx);
                av_seek_frame(v->input_ctx, v->video_stream, 0, AVSEEK_FLAG_BACKWARD);
                printf(" restarting video from time %.2f\n", v->current_time);
                continue;
            }
        } else if (ret < 0) {
            FAIL();
        }

        if (!done && packet.stream_index == v->video_stream) {
            if (avcodec_send_packet(v->decoder_ctx, &packet) < 0) FAIL();
            for (;;) {
                int ret = avcodec_receive_frame(v->decoder_ctx, v->frame);
                if (ret == AVERROR(EAGAIN)) break; // read new frame with av_read_frame and try decoding again
                else if (ret < 0) FAIL();

                int64_t pts = v->frame->best_effort_timestamp;
                if (pts == AV_NOPTS_VALUE) FAIL_WITH("No pts value. Unsupported video codec or corrupted file.");
                double frame_end_time = (pts + v->frame->pkt_duration)*v->video_time_base;
                assert(frame_end_time >= v->current_frame_end_time);

                found_frame = (v->current_time <= frame_end_time);

                if (found_frame) {
                    done = true;
                    v->current_frame_end_time = frame_end_time;

                    void*pix;
                    int pitch;
                    SDL_LockTexture(v->tex, NULL, &pix, &pitch);
                    sws_scale(v->sws_ctx, (uint8_t const * const *)v->frame->data,
                        v->frame->linesize, 0, v->decoder_ctx->height,
                        (uint8_t* []){pix, NULL, NULL}, (int [] ){pitch});
                    SDL_UnlockTexture(v->tex);
                    printf("new frame %.2f\n", frame_end_time);
                } else {
                    printf("skipping frame %.2f\n", frame_end_time);
                }

                if (done) break;
            }
        }
        av_packet_unref(&packet);

        if (done) break;
    }

    if (!found_frame) printf("\nno frame found\n");


    if (found_frame) {
        SDL_SetRenderDrawColor(context->renderer, 0, 0, 0, 255);
        SDL_RenderClear(context->renderer);
        SDL_RenderCopy(context->renderer, v->tex, NULL, &v->tex_dest);
        SDL_RenderPresent(context->renderer);
    }
    return found_frame;
}

void VideoClear(struct Video *v) {
    sws_freeContext(v->sws_ctx);
    av_frame_free(&v->frame);
    avcodec_close(v->decoder_ctx);
    avformat_close_input(&v->input_ctx);
    memset(v, 0, sizeof(*v));
    av_free(v);
}
