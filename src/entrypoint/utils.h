#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "boost/asio.hpp"
#include "common/coroutines/worker_pool.h"
#include "config.h"
#include "deduplicator/deduplicator_interface.h"
#include "directory/directory_interface.h"

#include "entrypoint/state.h"

namespace uh::cluster {

struct reference_collection {
    boost::asio::io_context& ioc;
    const services<deduplicator_interface>& dedupe_services;
    const services<directory_interface>& directory_services;
    state& server_state;
    entrypoint_config& config;
};

struct collapsed_objects {
    std::optional<std::string> _prefix{};
    std::optional<std::reference_wrapper<const object>> _object{};
};

struct retrieval {
    static std::vector<collapsed_objects>
    collapse(const std::vector<object>& objects,
             std::optional<std::string> delimiter,
             std::optional<std::string> prefix);
};

} // namespace uh::cluster

#endif
