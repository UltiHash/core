#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "boost/asio.hpp"
#include "common/registry/services.h"
#include "common/utils/worker_pool.h"
#include "config.h"
#include "entrypoint/state.h"

namespace uh::cluster {

struct reference_collection {
    boost::asio::io_context& ioc;
    const services<DEDUPLICATOR_SERVICE, coro_client>& dedupe_services;
    const services<DIRECTORY_SERVICE, coro_client>& directory_services;
    state& server_state;
    entrypoint_config& config;
};

struct collapsed_objects {
    std::optional<std::string> _prefix{};
    std::optional<std::reference_wrapper<const object>> _object{};
};

struct integration {
    static coro<dedupe_response> integrate_data(std::span<const char>,
                                                const reference_collection&);
};

struct retrieval {
    static std::vector<collapsed_objects>
    collapse(const std::vector<object>& objects,
             std::optional<std::string> delimiter,
             std::optional<std::string> prefix);
};

} // namespace uh::cluster

#endif
