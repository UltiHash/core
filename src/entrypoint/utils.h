#pragma once

#include <common/types/common_types.h>

#include <functional>
#include <optional>
#include <string>
#include <vector>

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
