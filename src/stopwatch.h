#pragma once

#include <chrono>

class Stopwatch {
    using chrono_clock = std::chrono::high_resolution_clock;
    chrono_clock clock{};
    chrono_clock::time_point start_time = clock.now();
public:
    auto ElapsedTime() const { return clock.now() - start_time; }
    void Restart() { start_time = clock.now(); }
};