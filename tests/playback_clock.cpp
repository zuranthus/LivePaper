#include <catch.hpp>
#include <thread>
#include <playback_clock.h>

using namespace std::chrono_literals;

namespace {
struct mock_clock {
    using time_point = std::chrono::high_resolution_clock::time_point;
    auto now() const { return time_point(time); }
    static std::chrono::milliseconds time;
};
std::chrono::milliseconds mock_clock::time{0ms};
void advance(std::chrono::milliseconds time) { mock_clock::time += time; }
using PlaybackClockTest = PlaybackClockBase<mock_clock>;
}

TEST_CASE("PlaybackClock", "[playback_clock]") {
    PlaybackClockTest clock(1000ms);
    SECTION("returns same time if time < duration") {
        advance(500ms);
        REQUIRE(clock.CurrentTime() == 500ms);
        advance(499ms);
        REQUIRE(clock.CurrentTime() == 999ms);
    }
    SECTION("loops time if over duration") {
        advance(1000ms);
        REQUIRE(clock.CurrentTime() == 0ms);
        advance(320ms);
        REQUIRE(clock.CurrentTime() == 320ms);
        advance(3010ms);
        REQUIRE(clock.CurrentTime() == 330ms);
    }
    SECTION("can restart") {
        advance(3659ms);
        clock.Restart();
        REQUIRE(clock.CurrentTime() == 0ms);
    }
}
