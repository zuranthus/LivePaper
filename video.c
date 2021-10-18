#include "fail.h"
#include <assert.h>

#include <SDL.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

static AVCodecContext *pCodecCtx = NULL;
static     AVFormatContext *pFormatCtx = NULL;
static struct SwsContext *sws_ctx = NULL;
static AVFrame *pFrame = NULL;
static int videoStream=-1;

void VideoLoad(char *path, int w, int h) {
    if(avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0)
        FAIL_WITH("Couldn't open file");

    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        FAIL_WITH("Couldn't find stream information");

    AVCodec *pCodec = NULL;
    videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);
    if (videoStream < 0) FAIL(); // Didn't find a video stream

    pCodecCtx = avcodec_alloc_context3(pCodec);
    assert(pCodecCtx);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
        FAIL();

    pFrame = av_frame_alloc();

    sws_ctx = sws_getContext(
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        w, h,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
}

void VideoNextFrame(SDL_Texture *tex) {
    for(AVPacket packet; av_read_frame(pFormatCtx, &packet) >= 0;) {
        if (packet.stream_index == videoStream) {
            int ret = avcodec_send_packet(pCodecCtx, &packet);
            if (ret < 0) FAIL();

            for (;;) {
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                else if (ret < 0) FAIL();

                void*pix;
                int pitch;
                SDL_LockTexture(tex, NULL, &pix, &pitch);
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height,
                    (uint8_t* []){pix, NULL, NULL}, (int [] ){pitch});
                SDL_UnlockTexture(tex);
            }
            break;
        }

        av_packet_unref(&packet);
    }
}

void VideoUnload() {
    sws_freeContext(sws_ctx);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}
