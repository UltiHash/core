//
// Created by masi on 7/18/23.
//

#ifndef CORE_BIG_INT_H
#define CORE_BIG_INT_H

#include <cstdint>
#include <string>

namespace uh::cluster {

class big_int {
    const uint64_t num[2];
public:
    constexpr big_int () noexcept: num {0,0} {
    }

    constexpr big_int (unsigned long number) noexcept: num {0, number} {
    }

    constexpr explicit big_int (std::string_view num_str): big_int () {

    }

    bool operator < (const big_int& other) const noexcept {
        return true;
    }

    bool operator == (const big_int& other) const noexcept {
        return true;
    }

    big_int operator+ (const big_int& other) const noexcept {
        return {};
    }

    big_int operator- (const big_int& other) const noexcept {
        return {};
    }

    [[nodiscard]] std::string to_string () const {
        return std::to_string(num[0]) + "_" + std::to_string(num[1]);
    }
};


} // end namespace uh::cluster
#endif //CORE_BIG_INT_H
