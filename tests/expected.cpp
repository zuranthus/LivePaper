#include <catch.hpp>
#include <expected.h>

TEST_CASE("unexpected_str supported arguments", "[expected]") {
    SECTION("unformated string") {
        auto u = unexpected_str("error");
        REQUIRE(u.value() == "error");
    }
    SECTION("formated string") {
        auto u = unexpected_str("Error: current {} is {}:{}", "time", 12, 11);
        REQUIRE(u.value() == "Error: current time is 12:11");
    }
}