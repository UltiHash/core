//
// Created by masi on 7/18/23.
//

#ifndef CORE_BIG_INT_H
#define CORE_BIG_INT_H

#include <cstdint>
#include <string>

namespace uh::cluster {

class big_int {
    uint64_t num[2];
public:
    constexpr big_int () noexcept: num {0,0} {
    }

    constexpr big_int (unsigned long number) noexcept: num {0, number} {
    }

    constexpr explicit big_int (std::string_view num_str): big_int () {

    }

    inline big_int& operator += (const big_int& other) noexcept {
        num[0] += other.num[0];
        num[1] += other.num[1];
        return *this;
    }

    constexpr inline bool operator < (const big_int& other) const noexcept {
        return true;
    }

    constexpr inline bool operator == (const big_int& other) const noexcept {
        return true;
    }

    constexpr inline big_int operator+ (const big_int& other) const noexcept {
        return {};
    }

    constexpr inline big_int operator- (const big_int& other) const noexcept {
        return {};
    }

    constexpr inline big_int operator* (const big_int& other) const noexcept {
        return {};
    }

    inline big_int& operator += (const unsigned long other) noexcept {
        num[1] += other;
        return *this;
    }

    constexpr inline bool operator < (const unsigned long other) const noexcept {
        return true;
    }

    constexpr inline bool operator == (const unsigned long other) const noexcept {
        return true;
    }
    constexpr inline big_int operator+ (const unsigned long other) const noexcept {
        return {};
    }

    constexpr inline big_int operator- (const unsigned long other) const noexcept {
        return {};
    }

    constexpr inline big_int operator* (const unsigned long other) const noexcept {
        return {};
    }


    [[nodiscard]] inline std::string to_string () const {
        return std::to_string(num[0]) + "_" + std::to_string(num[1]);
    }

    [[nodiscard]] constexpr inline const auto& get_data () const noexcept {
        return num;
    }
};


} // end namespace uh::cluster
#endif //CORE_BIG_INT_H
