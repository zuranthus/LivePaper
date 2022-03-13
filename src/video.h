#pragma once

#include <filesystem>
#include <memory>
#include "expected.h"


extern "C" {
    #include <libavformat/avformat.h>
}

class VideoStreamDecoder {
public:
    static auto FromFile(const std::filesystem::path& path) -> expected<VideoStreamDecoder>;

    VideoStreamDecoder(VideoStreamDecoder&&) = default;
    VideoStreamDecoder& operator=(VideoStreamDecoder&&) = default;

    auto width() { return context.avcodec->width; }
    auto height() { return context.avcodec->height; }
    auto frames() { return context.stream()->nb_frames; }

private:
    template <typename T>
    using raii_ptr = std::unique_ptr<T, std::function<void(T*)>>;

    struct Context {
        raii_ptr<AVFormatContext> avformat{};
        raii_ptr<AVCodecContext> avcodec{};
        int stream_id{};
        auto stream() -> AVStream* { return avformat->streams[stream_id]; }
    } context;

    explicit VideoStreamDecoder(Context&& context) : context(std::move(context)) {}
};
