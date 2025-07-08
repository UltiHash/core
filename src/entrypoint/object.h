#pragma once

#include <common/types/common_types.h>

#include <string>

namespace uh::cluster::ep {

enum class object_state {
    normal, deleted, collected
};

struct object {
    std::string name;
    utc_time last_modified;
    std::size_t size{};

    std::optional<address> addr;
    std::optional<std::string> etag;
    std::optional<std::string> mime;
    std::optional<std::string> version;

    object_state state = object_state::normal;

    constexpr static auto serialize(auto& archive, auto& self) {
        std::size_t count = 0;
        auto res = archive(self.name, count, self.size);

        self.last_modified = utc_time(utc_time::duration(count));
        return res;
    }

    constexpr static auto serialize(auto& archive, const auto& self) {
        std::size_t count = self.last_modified.time_since_epoch().count();
        return archive(self.name, count, self.size);
    }
};

object_state to_object_state(const std::string& s);
std::string to_string(object_state os);

} // namespace uh::cluster::ep
