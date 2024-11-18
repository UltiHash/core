#ifndef ENTRYPOINT_COMMON_H
#define ENTRYPOINT_COMMON_H

#include "common/types/common_types.h"
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

#endif
