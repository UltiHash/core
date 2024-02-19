#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "boost/asio.hpp"
#include "common/registry/services.h"
#include "entrypoint/rest/utils/state/server_state.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace uh::cluster {

struct entrypoint_state {
    boost::asio::io_context& ioc;
    boost::asio::thread_pool& workers;
    const services<DEDUPLICATOR_SERVICE>& dedupe_services;
    const services<DIRECTORY_SERVICE>& directory_services;
    rest::utils::server_state server_state;
};

struct integration {
    static coro<dedupe_response>
    integrate_data(const std::list<std::string_view>& data_pieces,
                   const entrypoint_state& state);
};

std::string generate_unique_id();

class command_unknown_exception : public std::exception {
  public:
    [[nodiscard]] const char* what() const noexcept override;
};

} // namespace uh::cluster

#endif
