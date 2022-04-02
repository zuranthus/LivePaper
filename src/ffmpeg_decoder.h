#pragma once

#include <filesystem>
#include <memory>
#include <chrono>
#include "expected.h"

extern "C" {
    #include <libavformat/avformat.h>
}

namespace ffmpeg_decoder {

using namespace std::chrono_literals;
using chrono_ms = std::chrono::milliseconds;
template <typename T>
using raii_ptr = std::unique_ptr<T, std::function<void(T*)>>;

class StreamDecoderContext {
    raii_ptr<AVFormatContext> avformat_{};
    raii_ptr<AVCodecContext> avcodec_{};
    int avstream_index_{};
    chrono_ms duration_;
public:
    StreamDecoderContext(raii_ptr<AVFormatContext> avformat, raii_ptr<AVCodecContext> avcodec,
        int avstream_id, chrono_ms duration)
    : avformat_(std::move(avformat)), avcodec_(std::move(avcodec))
    , avstream_index_(avstream_id), duration_(duration) {}
    StreamDecoderContext(StreamDecoderContext&&) = default;
    StreamDecoderContext& operator=(StreamDecoderContext&&) = default;

    auto avformat() { return avformat_.get(); }
    auto avcodec() { return avcodec_.get(); }
    auto avstream() { return avformat_->streams[avstream_index_]; }
    auto duration() { return duration_; }
};

class VideoDecoder {
    StreamDecoderContext context;
    bool finished{false};
    chrono_ms last_decoded_end_time{0ms};
public:
    struct Frame {
        raii_ptr<AVFrame> avframe{};
        chrono_ms start_time{-1ms};
        chrono_ms end_time{-1ms};
        auto width() { return avframe->width; }
        auto height() { return avframe->height; }
    };
    explicit VideoDecoder(StreamDecoderContext&& context);

    auto width() { return context.avcodec()->width; }
    auto height() { return context.avcodec()->height; }
    auto frames() { return context.avstream()->nb_frames; }
    auto duration() { return context.duration(); }

    bool HasFrames() const { return !finished; }
    auto NextFrame() -> expected<Frame>;
    auto SeekToFrameAt(chrono_ms time) -> expected<Frame>;
    auto Reset() -> expected<void>;
private:
    auto DecodeFrame(std::optional<chrono_ms> frame_time = std::nullopt) -> expected<Frame>;
};

class FileLoader {
    auto MakeError(std::string_view message) const;
    std::filesystem::path path;
public:
    FileLoader(const std::filesystem::path& path) : path(path) {}
    auto VideoStreamDecoder() -> expected<VideoDecoder>;
};
}
