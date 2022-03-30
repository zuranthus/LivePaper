#include <catch.hpp>

#include <decoding_loop.h>

#include <ffmpeg_decoder.h>

static auto test_path = std::filesystem::path("assets/test_4x2_16f_red.mp4");
// TODO add real tests
TEST_CASE("DecodingLoop loops?", "[decoding_loop]") {
    //Renderer renderer;
    auto r = std::make_unique<IRenderer>();
    DecodingLoop decoding_loop(*ffmpeg_decoder::FileLoader(test_path).VideoStreamDecoder(), r.get());
    REQUIRE(false);
}