#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "boost/asio.hpp"
#include "common/registry/services.h"
#include "entrypoint/state.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace uh::cluster {

struct reference_collection {
    boost::asio::io_context& ioc;
    boost::asio::thread_pool& workers;
    const services<DEDUPLICATOR_SERVICE>& dedupe_services;
    const services<DIRECTORY_SERVICE>& directory_services;
    state& server_state;
};

struct integration {
    static coro<dedupe_response>
    integrate_data(const std::list<std::string_view>&,
                   const reference_collection&);
};

} // namespace uh::cluster

#endif
