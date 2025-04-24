#pragma once

#include <string_view>
#include <vector>

namespace uh::cluster::storage {

struct group_config {
    int data_shards{0};
    int parity_shards{0};
    std::vector<int> members{};

    static group_config create(std::string_view json_str);
    static std::vector<group_config> create_multiple(std::string_view json_str);

    std::string to_string() const;
};

} // namespace uh::cluster::storage
