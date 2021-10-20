#include "fail.h"
#include <assert.h>
#include <stdbool.h>

#include <SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

static AVCodecContext *decoder_ctx = NULL;
static AVFormatContext *input_ctx = NULL;
static struct SwsContext *sws_ctx = NULL;
static AVFrame *frame = NULL;
static int video_stream=-1;
static double current_time = 0.0;
static double total_time = 1.0;
static double current_frame_end_time = -1.0;
static double video_time_base;

void VideoLoad(char *path, int w, int h) {
    if (avformat_open_input(&input_ctx, path, NULL, NULL)!=0)
        FAIL_WITH("Couldn't open file");
    if (avformat_find_stream_info(input_ctx, NULL)<0)
        FAIL_WITH("Couldn't find stream information");

    AVCodec *codec = NULL;
    video_stream = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (video_stream < 0) FAIL();

    decoder_ctx = avcodec_alloc_context3(codec);
    assert(decoder_ctx);
    AVStream *stream = input_ctx->streams[video_stream];
    avcodec_parameters_to_context(decoder_ctx, stream->codecpar);
    if(avcodec_open2(decoder_ctx, codec, NULL)<0)
        FAIL();
    video_time_base = av_q2d(stream->time_base);
    total_time = stream->duration*video_time_base;

    frame = av_frame_alloc();

    sws_ctx = sws_getContext(
        decoder_ctx->width,
        decoder_ctx->height,
        decoder_ctx->pix_fmt,
        w, h,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
}

bool VideoNextFrame(SDL_Texture *tex, double delta_sec) {
    current_time += delta_sec;

    printf("\nUPDATE %.3f/%.2f\n", current_time, current_frame_end_time);
    if (current_time < current_frame_end_time) {
        printf("skipping\n");
        return false;
    }

    bool done = false;
    bool found_frame = false;

    for(;;) {
        AVPacket packet;
        int ret = av_read_frame(input_ctx, &packet);
        if (ret == AVERROR_EOF) {
            printf("EOF ");
            if (current_time < total_time) {       
                done = true;
                printf("waiting for end of video\n");
            } else {
                current_time = fmod(current_time, total_time);
                current_frame_end_time = -1.0;
                avcodec_flush_buffers(decoder_ctx);
                av_seek_frame(input_ctx, video_stream, 0, AVSEEK_FLAG_BACKWARD);
                printf(" restarting video from time %.2f\n", current_time);
                continue;
            }
        } else if (ret < 0) {
            FAIL();
        }

        if (!done && packet.stream_index == video_stream) {
            if (avcodec_send_packet(decoder_ctx, &packet) < 0) FAIL();
            for (;;) {
                int ret = avcodec_receive_frame(decoder_ctx, frame);
                if (ret == AVERROR(EAGAIN)) break; // read new frame with av_read_frame and try decoding again
                else if (ret < 0) FAIL();

                int64_t pts = frame->best_effort_timestamp;
                if (pts == AV_NOPTS_VALUE) FAIL_WITH("No pts value. Unsupported video codec or corrupted file.");
                double frame_end_time = (pts + frame->pkt_duration)*video_time_base;
                assert(frame_end_time >= current_frame_end_time);

                found_frame = (current_time <= frame_end_time);

                if (found_frame) {
                    done = true;
                    current_frame_end_time = frame_end_time;

                    void*pix;
                    int pitch;
                    SDL_LockTexture(tex, NULL, &pix, &pitch);
                    sws_scale(sws_ctx, (uint8_t const * const *)frame->data,
                        frame->linesize, 0, decoder_ctx->height,
                        (uint8_t* []){pix, NULL, NULL}, (int [] ){pitch});
                    SDL_UnlockTexture(tex);
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

    return found_frame;
}

void VideoUnload() {
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    avcodec_close(decoder_ctx);
    avformat_close_input(&input_ctx);
}
