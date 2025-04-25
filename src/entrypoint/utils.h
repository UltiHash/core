#pragma once

#include "common/service_interfaces/deduplicator_interface.h"
#include "config.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"
#include "entrypoint/multipart_state.h"
#include "storage/global/data_view.h"

#include <boost/asio.hpp>
#include <boost/url/url.hpp>

namespace uh::cluster {

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
