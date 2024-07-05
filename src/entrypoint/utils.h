#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "common/global_data/global_data_view.h"
#include "common/registry/services.h"
#include "config.h"
#include "deduplicator/interfaces/deduplicator_interface.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"
#include "entrypoint/multipart_state.h"

#include <boost/asio.hpp>
#include <boost/url/url.hpp>

namespace uh::cluster {

struct reference_collection {
    boost::asio::io_context& ioc;
    const services<deduplicator_interface>& dedupe_services;
    uh::cluster::directory& directory;
    multipart_state& uploads;
    entrypoint_config& config;
    global_data_view& gdv;
    uh::cluster::limits& limits;
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

/**
 * Return bucket and object key.
 */
std::tuple<std::string, std::string>
extract_bucket_and_object(boost::urls::url url);

} // namespace uh::cluster

#endif
