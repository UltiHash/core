#pragma once

#include <boost/log/sources/record_ostream.hpp>
#include <cstdint>
#include <format>
#include <functional>
#include <iomanip>
#include <iosfwd>
#include <string>

using uint128_t = __uint128_t;

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

namespace boost {
namespace log {

template <typename CharT>
inline boost::log::basic_record_ostream<CharT>&
operator<<(boost::log::basic_record_ostream<CharT>& os,
           const __uint128_t& value) {
    std::ostream& stream = os.stream();
    ::operator<<(stream, value);
    return os;
}

} // namespace log
} // namespace boost
