#pragma once

#include <entrypoint/object.h>
#include <entrypoint/http/response.h>

#include <optional>
#include <string>
#include <vector>

namespace uh::cluster {

struct collapsed_objects {
    std::optional<std::string> _prefix{};
    std::optional<std::reference_wrapper<const ep::object>> _object{};
};

struct retrieval {
    static std::vector<collapsed_objects>
    collapse(const std::vector<ep::object>& objects,
             std::optional<std::string> delimiter,
             std::optional<std::string> prefix);
};

using encoder_function = std::optional<std::string> (*)(std::optional<std::string>);
encoder_function encoder(std::optional<std::string> encoding_type);

/**
 * Set default response headers from object.
 */
void set_default_headers(ep::http::response& res, const ep::object& obj);

} // namespace uh::cluster
