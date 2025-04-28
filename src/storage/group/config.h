#pragma once

#include <string_view>
#include <vector>

namespace uh::cluster::storage {

struct group_config {
    // TODO: Initialize them using {}.
    std::size_t data_shards{1};
    std::size_t parity_shards{0};
    std::vector<std::size_t> members{0};

    static group_config create(std::string_view json_str);
    static std::vector<group_config> create_multiple(std::string_view json_str);

    std::string to_string() const;
};

} // namespace uh::cluster::storage
