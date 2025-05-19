#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <iomanip>
#include <iosfwd>
#include <string>

namespace uh::cluster {

using uint128_t = __uint128_t;

} // end namespace uh::cluster

inline std::ostream& operator<<(std::ostream& os, const __uint128_t& value) {
    constexpr int num_blocks = 8;
    constexpr __uint128_t mask = 0xFFFF;

    for (int i = num_blocks - 1; i >= 0; --i) {
        uint16_t block = static_cast<uint16_t>((value >> (i * 16)) & mask);
        if (i != num_blocks - 1) {
            os << ":";
        }
        os << std::hex << std::setfill('0') << std::setw(4) << block;
    }

    return os;
}

template <> struct std::formatter<__uint128_t> {
    char presentation = 'x';

    constexpr auto parse(std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 'x' || *it == 'd')) {
            presentation = *it++;
        }
        if (it != ctx.end() && *it != '}') {
            throw std::format_error("invalid format");
        }
        return it;
    }

    auto format(const __uint128_t& value, std::format_context& ctx) const {
        auto out = ctx.out();
        if (presentation == 'x') {
            uint64_t high = static_cast<uint64_t>(value >> 64);
            uint64_t low = static_cast<uint64_t>(value);
            if (high > 0) {
                return std::format_to(out, "{:x}{:016x}", high, low);
            } else {
                return std::format_to(out, "{:x}", low);
            }
        } else if (presentation == 'd') {
            std::string result;
            __uint128_t temp = value;
            do {
                result.insert(result.begin(),
                              '0' + static_cast<char>(temp % 10));
                temp /= 10;
            } while (temp > 0);
            return std::format_to(out, "{}", result);
        }
        throw std::format_error("unsupported format");
    }
};

// namespace std {
// template <> struct is_arithmetic<__uint128_t> : std::true_type {};
// } // namespace std

// template <>
// struct std::hash<__uint128_t> {
//     std::size_t operator()(const __uint128_t& value) const noexcept {
//         uint64_t high = static_cast<uint64_t>(value >> 64);
//         uint64_t low = static_cast<uint64_t>(value);
//
//         return std::hash<uint64_t>{}(high) ^ (std::hash<uint64_t>{}(low) <<
//         1);
//     }
// };
