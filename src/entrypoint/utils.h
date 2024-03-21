#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "boost/asio.hpp"
#include "common/registry/services.h"
#include "common/utils/worker_pool.h"
#include "entrypoint/state.h"

namespace uh::cluster {

struct reference_collection {
    boost::asio::io_context& ioc;
    worker_pool& workers;
    const services<DEDUPLICATOR_SERVICE>& dedupe_services;
    const services<DIRECTORY_SERVICE>& directory_services;
    state& server_state;
};

struct collapsed_objects {
    std::optional<std::string> _prefix{};
    std::optional<std::reference_wrapper<const object>> _object{};
};

struct integration {
    static coro<dedupe_response>
    integrate_data(const std::list<std::string_view>&,
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
