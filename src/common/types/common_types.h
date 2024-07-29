#ifndef CORE_COMMON_TYPES_H
#define CORE_COMMON_TYPES_H

#include "address.h"
#include "big_int.h"
#include <boost/asio.hpp>
#include <chrono>
#include <zpp_bits.h>

namespace uh::cluster {

static constexpr std::size_t KIBI_BYTE = 1024;
static constexpr std::size_t MEBI_BYTE = 1024 * KIBI_BYTE;
static constexpr std::size_t GIBI_BYTE = 1024 * MEBI_BYTE;
static constexpr std::size_t TEBI_BYTE = 1024 * GIBI_BYTE;
static constexpr std::size_t PEBI_BYTE = 1024 * TEBI_BYTE;

struct dedupe_response {
    std::size_t effective_size{};
    address addr;

    void append(const dedupe_response& other) {
        effective_size += other.effective_size;
        addr.append(other.addr);
    }
};

using utc_time = std::chrono::time_point<std::chrono::system_clock>;

struct object {
    std::string name;
    utc_time last_modified;
    std::size_t size{};

    std::optional<address> addr;
    std::optional<std::string> etag;
    std::optional<std::string> mime;

    constexpr static auto serialize(auto& archive, auto& self) {
        std::size_t count = 0;
        auto res = archive(self.name, count, self.size);

        self.last_modified = utc_time(utc_time::duration(count));
        return res;
    }

    constexpr static auto serialize(auto& archive, const auto& self) {
        std::size_t count = self.last_modified.time_since_epoch().count();
        return archive(self.name, count, self.size);
    }
};

template <typename T> using coro = boost::asio::awaitable<T>;

} // end namespace uh::cluster

#endif // CORE_COMMON_TYPES_H
