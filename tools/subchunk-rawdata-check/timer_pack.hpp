//
// Created by Masoud Gholami on 01.02.22.
//

#ifndef EVALUATION_TIMER_PACK_HPP
#define EVALUATION_TIMER_PACK_HPP

#include <array>
#include <chrono>

template <int ntimers = 1>
class timer_pack {
private:
    using clock = std::chrono::system_clock;
    std::array <std::chrono::time_point <clock>, ntimers> timers {};
    std::array <std::chrono::duration <double>, ntimers> total_durations {};
    std::array <std::chrono::duration <double>, ntimers> durations {};
    std::array <long, ntimers> counts {};
public:

    constexpr inline void start (int id) noexcept {
        timers.at (id) = clock::now ();
    }

    constexpr inline void stop (int id) noexcept {
        auto now = clock::now ();
        durations.at (id) = now - timers.at (id);
        total_durations.at (id) += durations.at (id);
        counts.at (id)++;
    }

    constexpr inline double total_duration (int id) {
        return total_durations.at (id).count ();
    }

    constexpr inline double avg_duration (int id) {
        return total_durations.at (id).count () / counts.at (id);
    }

    constexpr inline double duration (int id) {
        return durations.at (id).count ();
    }

    constexpr inline double live_duration (int id) {
        return std::chrono::duration <double> (clock::now () - timers.at (id)).count();
    }

    constexpr inline long count (int id) {
        return counts.at (id);
    }

    constexpr inline void reset (int id) {
        counts.at (id) = 0;
        durations.at (id) = {};
        total_durations.at (id) = {};
        timers.at (id) = {};
    }

};
#endif //EVALUATION_TIMER_PACK_HPP
