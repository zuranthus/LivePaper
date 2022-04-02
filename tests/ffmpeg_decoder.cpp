#include <catch.hpp>
#include <expected.h>
#include <filesystem>

#include <ffmpeg_decoder.h>


using namespace ffmpeg_decoder;
using namespace std::literals::chrono_literals;

static auto test_path = std::filesystem::path("assets/test_4x2_16f_red.mp4");

//TODO check for memory leaks in tests
namespace Catch {
    template<typename T>
    struct StringMaker<expected<T>> {
        static std::string convert(const expected<T>& value) {
            return value 
                ? StringMaker<decltype(value.value())>::convert(value.value())
                : value.error();
        }
    };
    template<>
    struct StringMaker<expected<void>> {
        static std::string convert(const expected<void>& value) {
            return value ? "void" : value.error();
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

void CheckFrameContainsRed(AVFrame& frame) {
    auto y = frame.data[0][0];
    auto u = frame.data[1][0];
    auto v = frame.data[2][0];
    // check that the frame contains red color
    // (255, 0, 0)_rgb is approximately (81, 90, 240)_yuv
    //TODO: check the whole frame instead of 1 pixel
    REQUIRE(abs(y - 81) <= 1);
    REQUIRE(abs(u - 90) <= 1);
    REQUIRE(abs(v - 240) <= 1);
}

void CheckFrame25Fps(VideoDecoder::Frame& frame, int frame_number) {
    const auto ms_per_frame = 1000ms/25;
    CheckFrameContainsRed(*frame.avframe);
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

TEST_CASE("VideoDecoder duration is correct", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    REQUIRE(decoder->duration() == 640ms);
}

auto check_seek_frame_func(auto& decoder) {
    return [&decoder](auto time) {
        auto frame = decoder->SeekToFrameAt(time);
        REQUIRE(frame);
        REQUIRE(time >= frame->start_time);
        REQUIRE(time < frame->end_time);
        CheckFrameContainsRed(*frame->avframe);
        return frame;
    };
}

TEST_CASE("VideoDecoder seeks to frame start times", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    auto time = GENERATE(40ms, 320ms, 600ms);
    check_seek_frame_func(decoder)(time);
}

TEST_CASE("VideoDecoder seeks to frame end times", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    auto time = GENERATE(79ms, 279ms, 639ms);
    check_seek_frame_func(decoder)(time);
}

TEST_CASE("VideoDecoder seeks forward", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    auto check_seek_frame = check_seek_frame_func(decoder);
    check_seek_frame(100ms);
    check_seek_frame(200ms);
    check_seek_frame(639ms);
}

TEST_CASE("VideoDecoder seeks backward", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    auto check_seek_frame = check_seek_frame_func(decoder);
    check_seek_frame(500ms);
    check_seek_frame(300ms);
    check_seek_frame(200ms);
    check_seek_frame(639ms);
    check_seek_frame(0ms);
}

TEST_CASE("VideoDecoder reports invalid seek times", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    auto time = GENERATE(-1ms, -34s, 640ms, 60s);
    auto frame = decoder->SeekToFrameAt(time);
    REQUIRE(!frame);
}

TEST_CASE("VideoDecoder seeks after restart", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    decoder->SeekToFrameAt(500ms);
    decoder->Reset();
    check_seek_frame_func(decoder)(100ms);
}

TEST_CASE("VideoDecoder seeks and next frame work together", "[ffmpeg_decoder]") {
    auto decoder = FileLoader(test_path).VideoStreamDecoder();
    decoder->NextFrame();
    auto seek_frame = check_seek_frame_func(decoder)(160ms);
    auto next_frame = decoder->NextFrame();
    REQUIRE(next_frame);
    REQUIRE(next_frame->start_time == seek_frame->end_time);
    CheckFrameContainsRed(*next_frame->avframe);
}
