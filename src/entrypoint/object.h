#pragma once

#include <common/types/common_types.h>

#include <string>

namespace uh::cluster::ep {

enum class object_state {
    normal, deleted, collected
};

struct object {
    uint64_t id;
    std::string name;
    std::size_t size{};
    utc_time last_modified;

    std::optional<address> addr;
    std::optional<std::string> etag;
    std::optional<std::string> mime;
    std::optional<std::string> version;

    object_state state = object_state::normal;
};

object_state to_object_state(const std::string& s);
std::string to_string(object_state os);

} // namespace uh::cluster::ep
