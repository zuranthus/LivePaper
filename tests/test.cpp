#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <video.h>
#include <expected.h>
#include <filesystem>

using namespace ffmpeg_decoder;

TEST_CASE("make_unexpected_str supported arguments", "[expected]") {
    SECTION("unformated string") {
        auto u = make_unexpected_str("error");
        REQUIRE(u.value() == "error");
    }
    SECTION("formated string") {
        auto u = make_unexpected_str("Error: current {} is {}:{}", "time", 12, 11);
        REQUIRE(u.value() == "Error: current time is 12:11");
    }
}

TEST_CASE("FileLoader reports errors for invalid files", "[ffmpeg_decoder]") {
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

TEST_CASE("FileLoader loads videos", "[ffmpeg_decoder]") {
    auto path = GENERATE("assets/test.mp4", "assets/test.gif");
    auto video = FileLoader(path).VideoStreamDecoder();
    REQUIRE(video);
    REQUIRE(video->width() == 1000);
    REQUIRE(video->height() == 562);
    REQUIRE(video->frames() == 32);
}