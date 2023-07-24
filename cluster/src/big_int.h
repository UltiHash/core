//
// Created by masi on 7/18/23.
//

#ifndef CORE_BIG_INT_H
#define CORE_BIG_INT_H

#include <cstdint>
#include <string>

namespace uh::cluster {

class big_int {
    uint64_t num[2];  // 0 = high, 1 = low
public:
    constexpr big_int () noexcept: num {0,0} {
    }

    constexpr big_int (unsigned long number) noexcept: num {0, number} {
    }

    constexpr big_int (unsigned long nh, unsigned long nl) noexcept: num {nh, nl} {
    }

    explicit big_int (const std::string& num_str): big_int () {
        const auto index = num_str.find('_') + 1;
        const auto num0_str = num_str.substr(0, index - 1);
        const auto num1_str = num_str.substr(index, num_str.size());
        num[0] = std::stoul(num0_str);
        num[1] = std::stoul(num1_str);
    }

    inline big_int& operator += (const big_int& other) noexcept {
        num[0] += other.num[0];
        num[1] += other.num[1];
        return *this;
    }

    constexpr inline bool operator < (const big_int& other) const noexcept {
        return num[0] < other.num[0] or ((num[0] == other.num[0]) and (num[1] < other.num[1]));
    }

    constexpr inline bool operator == (const big_int& other) const noexcept {
        return num[1] == other.num[1] and num[0] == other.num[0];
    }

    constexpr inline big_int operator+ (const big_int& other) const noexcept {
        big_int res {num[0] + other.num[0], num[1] + other.num[1]};
        const auto max_no_overflow = UNSIGNED_MAX - num[1];
        if (other.num[1] > max_no_overflow) [[unlikely]] {
            res.num[1] = other.num[1] - max_no_overflow;
            res.num[0] ++;
        }
        return res;
    }

    constexpr inline big_int operator- (const big_int& other) const noexcept {
        big_int res {num[0] - other.num[0], num[1] - other.num[1]};
        const auto max_no_underflow = UNSIGNED_MAX - num[1];
        if (other.num[1] > max_no_underflow) [[unlikely]] {
            res.num[1] = UNSIGNED_MAX - other.num[1] + max_no_underflow;
            res.num[0] --;
        }
        return res;
    }


    constexpr inline bool operator < (const unsigned long other) const noexcept {
        return num[0] == 0 and num[1] < other;
    }

    constexpr inline bool operator == (const unsigned long other) const noexcept {
        return num[0] == 0 and num[1] == other;
    }
    constexpr inline big_int operator+ (const unsigned long other) const noexcept {
        big_int res {num[0], num[1] + other};
        const auto max_no_overflow = UNSIGNED_MAX - num[1];
        if (other > max_no_overflow) [[unlikely]] {
            res.num[1] = other - max_no_overflow;
            res.num[0] ++;
        }
        return res;
    }

    constexpr inline big_int operator- (const unsigned long other) const noexcept {
        big_int res {num[0], num[1] - other};
        const auto max_no_underflow = UNSIGNED_MAX - num[1];
        if (other > max_no_underflow) [[unlikely]] {
            res.num[1] = UNSIGNED_MAX - other + max_no_underflow;
            res.num[0] --;
        }
        return res;
    }

    constexpr inline big_int operator* (const unsigned long other) const noexcept {
        return {};
    }

    inline big_int& operator += (const unsigned long other) noexcept {
        const auto max_no_overflow = UNSIGNED_MAX - num[1];
        if (other > max_no_overflow) [[unlikely]] {
            num[1] = other - max_no_overflow;
            num[0] ++;
        }
        else {
            num[1] += other;
        }
        return *this;
    }

    [[nodiscard]] inline std::string to_string () const {
        return std::to_string(num[0]) + "_" + std::to_string(num[1]);
    }

    [[nodiscard]] constexpr inline const auto& get_data () const noexcept {
        return num;
    }

    constexpr static auto UNSIGNED_MAX = std::numeric_limits <unsigned long>::max();

};


} // end namespace uh::cluster
#endif //CORE_BIG_INT_H
