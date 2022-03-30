#pragma once

#include <gsl/gsl>
#include "ffmpeg_decoder.h"

class IRenderer {

};

class DecodingLoop {
public:
    DecodingLoop(ffmpeg_decoder::VideoDecoder&& decoder, gsl::not_null<IRenderer*> renderer)
    : decoder(std::move(decoder)), renderer(renderer) {}

private:
    ffmpeg_decoder::VideoDecoder decoder;
    gsl::not_null<IRenderer*> renderer;
};