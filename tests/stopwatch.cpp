#include <catch.hpp>
#include <thread>
#include <stopwatch.h>

using namespace std::chrono_literals;

TEST_CASE("Stopwatch resets", "[stopwatch]") {
    Stopwatch stopwatch;
    REQUIRE(stopwatch.ElapsedTime() < 100ms);
    std::this_thread::sleep_for(150ms);
    REQUIRE(stopwatch.ElapsedTime() > 100ms);
    stopwatch.Restart();
    REQUIRE(stopwatch.ElapsedTime() < 100ms);
}
