#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <video.h>
#include <expected.h>
#include <filesystem>

TEST_CASE("make_unexpected_str supported arguments") {
    SECTION("unformated string") {
        auto u = make_unexpected_str("error");
        REQUIRE(u.value() == "error");
    }
    SECTION("formated string") {
        auto u = make_unexpected_str("Error: current {} is {}:{}", "time", 12, 11);
        REQUIRE(u.value() == "Error: current time is 12:11");
    }
}

TEST_CASE("VideoStreamDecoder::FromFile reports errors for invalid filenames") {
    std::filesystem::path path;
    SECTION("nonexistent file") {
        path = "assets/nonexistent";
        REQUIRE(!std::filesystem::exists(path));
    }
    SECTION("invalid file") {
        path = "assets/invalid.mp4";
        REQUIRE(std::filesystem::exists(path));
    }
    auto decoder = VideoStreamDecoder::FromFile(path);
    REQUIRE(!decoder);
    REQUIRE(decoder.error().contains(path.string()));
}

TEST_CASE("VideoStreamDecoder::FromFile loads video file") {
    auto path = GENERATE("assets/test.mp4", "assets/test.gif");
    auto video = VideoStreamDecoder::FromFile(path);
    REQUIRE(video);
    REQUIRE(video->width() == 1000);
    REQUIRE(video->height() == 562);
    REQUIRE(video->frames() == 32);
}