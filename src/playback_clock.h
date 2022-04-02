#pragma once

#include <chrono>

template <typename base_clock>
class PlaybackClockBase {
    base_clock clock;
    base_clock::time_point start_time{clock.now()};
    std::chrono::milliseconds video_duration;
public:
    explicit PlaybackClockBase(std::chrono::milliseconds video_duration) 
    : video_duration(video_duration) {}
    auto CurrentTime() const { return (clock.now() - start_time)%video_duration; }
    void Restart() { start_time = clock.now(); }
};

using PlaybackClock = PlaybackClockBase<std::chrono::high_resolution_clock>;