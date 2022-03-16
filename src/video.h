#pragma once

#include <filesystem>
#include <memory>
#include "expected.h"


extern "C" {
    #include <libavformat/avformat.h>
}

namespace ffmpeg_decoder {

class VideoDecoder;

struct StreamDecoderContext {
    template <typename T>
    using raii_ptr = std::unique_ptr<T, std::function<void(T*)>>;

    raii_ptr<AVFormatContext> avformat{};
    raii_ptr<AVCodecContext> avcodec{};
    int stream_id{};
    auto stream() const -> AVStream* { return avformat->streams[stream_id]; }
};

class FileLoader {
public:
    FileLoader(const std::filesystem::path& path) : path(path) {}
    auto VideoStreamDecoder() -> expected<VideoDecoder>;
private:
    auto MakeError(std::string_view message) const;
    std::filesystem::path path;
};

class VideoDecoder {
public:
    explicit VideoDecoder(StreamDecoderContext&& context) : context(std::move(context)) {}
    VideoDecoder(VideoDecoder&&) = default;
    VideoDecoder& operator=(VideoDecoder&&) = default;

    auto width() const { return context.avcodec->width; }
    auto height() const { return context.avcodec->height; }
    auto frames() const { return context.stream()->nb_frames; }
private:
    StreamDecoderContext context;
};


}