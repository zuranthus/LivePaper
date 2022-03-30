#include <catch.hpp>
#include <expected.h>
#include <filesystem>

#include <ffmpeg_decoder.h>

using namespace ffmpeg_decoder;
using namespace std::literals::chrono_literals;

static auto test_path = std::filesystem::path("assets/test_4x2_16f_red.mp4");

namespace Catch {
    template<typename T>
    struct StringMaker<expected<T>> {
        static std::string convert( expected<T> const& value ) {
            return value 
                ? StringMaker<decltype(value.value())>::convert(value.value())
                : value.error();
        }
    };
    template<>
    struct StringMaker<expected<void>> {
        static std::string convert( expected<void> const& value ) {
            return value 
                ? "void"
                : value.error();
        }
    };
}

TEST_CASE("FileLoader reports error for invalid files", "[ffmpeg_decoder]") {
    std::filesystem::path path;
    SECTION("nonexistent file") {
        path = "assets/nonexistent";
        REQUIRE(!std::filesystem::exists(path));
    }
    SECTION("invalid file") {
        path = "assets/invalid.mp4";
        REQUIRE(std::filesystem::exists(path));
    }
    auto decoder = FileLoader(path).VideoStreamDecoder();
    REQUIRE(!decoder);
    REQUIRE(decoder.error().contains(path.string()));
}

TEST_CASE("FileLoader creates decoders for valid files", "[ffmpeg_decoder]") {
    auto path = GENERATE("assets/test_4x2_16f_red.mp4", "assets/test.mp4", "assets/test.gif");
    REQUIRE(FileLoader(path).VideoStreamDecoder());
}

TEST_CASE("VideoDecoder provides correct stream info", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    REQUIRE(decoder);
    REQUIRE(decoder->width() == 4);
    REQUIRE(decoder->height() == 2);
    REQUIRE(decoder->frames() == 16);
}

TEST_CASE("VideoDecoder decodes frame", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    REQUIRE(decoder->HasFrames());
    auto frame = decoder->NextFrame();
    REQUIRE(frame);
}

void CheckFrameContainsRed(AVFrame* frame) {
    auto y = frame->data[0][0];
    auto u = frame->data[1][0];
    auto v = frame->data[2][0];
    // check that the frame contains red color
    // (255, 0, 0)_rgb is approximately (81, 90, 240)_yuv
    //TODO: check the whole frame instead of 1 pixel
    REQUIRE(abs(y - 81) <= 1);
    REQUIRE(abs(u - 90) <= 1);
    REQUIRE(abs(v - 240) <= 1);
}

void CheckFrame25Fps(VideoDecoder::Frame& frame, int frame_number) {
    const auto ms_per_frame = 1000ms/25;
    CheckFrameContainsRed(frame.avframe);
    REQUIRE(frame.start_time == frame_number*ms_per_frame);
    REQUIRE(frame.end_time == (frame_number + 1)*ms_per_frame);
}

TEST_CASE("VideoDecoder decodes all frames correctly", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    auto frame_count = 0;
    while (decoder->HasFrames()) {
        if (auto frame = decoder->NextFrame()) {
            CheckFrame25Fps(*frame, frame_count);
            ++frame_count;
        }
    }
    REQUIRE(frame_count == 16);
}

TEST_CASE("VideoDecoder returns no frames after decoding finishes", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    while (decoder->HasFrames()) decoder->NextFrame();
    REQUIRE(!decoder->HasFrames());
    REQUIRE(!decoder->NextFrame());
}

TEST_CASE("VideoDecoder can restart after decoding finishes", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    while (decoder->HasFrames()) decoder->NextFrame();
    REQUIRE(decoder->Reset());
    REQUIRE(decoder->HasFrames());
    auto frame = decoder->NextFrame();
    REQUIRE(frame);
    CheckFrame25Fps(*frame, 0);
    REQUIRE(frame->start_time == 0ms);
    REQUIRE(frame->end_time == 40ms);
}

TEST_CASE("VideoDecoder can restart multiple times", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    int repetitions = 5;
    while (repetitions--) {
        for (int f = 0; f < 9; ++f) {
            REQUIRE(decoder->HasFrames());
            auto frame = decoder->NextFrame();
            REQUIRE(frame);
            CheckFrame25Fps(*frame, f);
        }
        REQUIRE(decoder->Reset());
    }
}