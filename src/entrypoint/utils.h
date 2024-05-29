#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "boost/asio.hpp"
#include "common/global_data/global_data_view.h"
#include "common/registry/services.h"
#include "config.h"
#include "deduplicator/interfaces/deduplicator_interface.h"
#include "entrypoint/directory.h"
#include "entrypoint/state.h"

#include <atomic>

namespace uh::cluster {

struct reference_collection {
    boost::asio::io_context& ioc;
    const services<deduplicator_interface>& dedupe_services;
    uh::cluster::directory& directory;
    state& server_state;
    entrypoint_config& config;
    global_data_view& gdv;
    std::atomic<std::size_t>& data_storage_size;
    std::size_t max_data_size;

    // warn about a nearly reached size limit at this percentage
    static constexpr unsigned SIZE_LIMIT_WARNING_PERCENTAGE = 80;
    // number of files to upload between two warnings
    static constexpr unsigned SIZE_LIMIT_WARNING_INTERVAL = 100;

    void check_storage_size(std::size_t increment) const;
    void free_storage_size(std::size_t decrement) const;
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
