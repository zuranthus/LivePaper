#include "fail.h"
#include <assert.h>

#include <SDL.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

static AVCodecContext *pCodecCtx = NULL;
static AVCodecContext *pCodecCtxOrig = NULL;
static     AVFormatContext *pFormatCtx = NULL;
static     AVFrame *pFrame = NULL;
static struct SwsContext *sws_ctx = NULL;
static int videoStream=-1;

void VideoLoad(char *path, int w, int h) {
    // Open video file
    if(avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0)
        FAIL_WITH("Couldn't open file");

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        FAIL_WITH("Couldn't find stream information");

    int i;


    // Find the first video stream
    for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
        videoStream=i;
        break;
    }
    if(videoStream==-1)
        FAIL(); // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;
    assert(pCodecCtxOrig);

    AVCodec *pCodec = NULL;

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if(pCodec==NULL) {
        FAIL_WITH("Unsupported codec!\n");
    }
    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
        FAIL_WITH("Couldn't copy codec context");
    }
    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
        FAIL();

    // Allocate video frame
    pFrame=av_frame_alloc();
    assert(pFrame);
    
    // initialize SWS context for software scaling
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
    int i=0;
    AVPacket packet;
    int frameFinished = 0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
           
            // Did we get a video frame?
            if(frameFinished) {
                void*pix;
                int pitch;
                SDL_LockTexture(tex, NULL, &pix, &pitch);
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height,
                    (uint8_t* []){pix, NULL, NULL}, (int [] ){pitch});
               SDL_UnlockTexture(tex);
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        if (frameFinished) break;
    }
}

void VideoUnload() {
    // Free the YUV frame
    av_free(pFrame);

    // Close the codecs
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // Close the video file
    avformat_close_input(&pFormatCtx);
}
