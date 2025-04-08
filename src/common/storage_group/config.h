#pragma once

#include <string_view>
#include <vector>

namespace uh::cluster::storage_group {

struct config {
    int data_shards{0};
    int parity_shards{0};
    std::vector<int> members{};

    static config create(std::string_view json_str);
    static std::vector<config> create_multiple(std::string_view json_str);

    std::string to_string() const;
};

} // namespace uh::cluster::storage_group
