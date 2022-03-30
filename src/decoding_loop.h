#pragma once

#include <chrono>
#include <gsl/gsl>
#include "ffmpeg_decoder.h"

using namespace std::chrono_literals;
// TODO implement renderer
class IRenderer {

};
// TODO separate in several single-responsibility classes
class DecodingLoop {
    struct {
        std::chrono::milliseconds end_time{-1ms};
        std::optional<ffmpeg_decoder::VideoDecoder::Frame> next{};
    } frame;
public:
    DecodingLoop(ffmpeg_decoder::VideoDecoder&& decoder, gsl::not_null<IRenderer*> renderer)
    : decoder(std::move(decoder)), renderer(renderer) {}

    void Loop() {
        std::chrono::high_resolution_clock clock;
        frame.next = decoder.NextFrame().value();
        auto start_time = clock.now();
        while (true) {
            auto playback_pos = clock.now() - start_time;
            if (playback_pos >= frame.end_time) {
                RenderFrame(frame.next.value());
                frame.end_time = frame.next->end_time;
                frame.next.reset(); 
                // TODO find next frame
            }
        }
    }

    void RenderFrame(ffmpeg_decoder::VideoDecoder::Frame& frame) {
        //TODO add rendering code
       // renderer.Render(frame.avframe ...)  ...

    }



private:
    ffmpeg_decoder::VideoDecoder decoder;
    gsl::not_null<IRenderer*> renderer;
};