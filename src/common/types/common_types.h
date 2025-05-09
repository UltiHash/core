#pragma once

#include "address.h"
#include "scoped_buffer.h"

#include <common/telemetry/trace/trace_asio.h>

#include <boost/asio/awaitable.hpp>
#include <chrono>

namespace uh::cluster {

struct dedupe_response {
    std::size_t effective_size{};
    address addr;

    void append(const dedupe_response& other) {
        effective_size += other.effective_size;
        addr.append(other.addr);
    }
};

struct allocation_t {
    std::size_t offset;
    std::size_t size;
};

struct write_request {
    allocation_t allocation;
    std::variant<std::span<const char>, unique_buffer<>> data;
    std::vector<std::size_t> offsets;
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

template <typename T> using coro = boost::asio::traced_awaitable<T>;

template <typename T> using optref = std::optional<std::reference_wrapper<T>>;

inline thread_local opentelemetry::context::Context THREAD_LOCAL_CONTEXT;

} // end namespace uh::cluster
