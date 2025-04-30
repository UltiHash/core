#pragma once

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <string_view>
#include <vector>

namespace uh::cluster::storage {

struct group_config {
    enum type_t { ROUND_ROBIN, ERASURE_CODING, REPLICA };

    std::size_t id{};
    type_t type{};
    // TODO: remove default value after getting config using ENV VAR
    std::size_t storages{3};
    std::size_t data_shards{};
    std::size_t parity_shards{};

    static group_config create(std::string_view json_str);
    std::string to_string() const;
};

struct group_configs {
    // TODO: remove default value after getting config using ENV VAR
    std::vector<group_config> configs{{}};

    group_configs() = default;

    explicit group_configs(std::vector<group_config>&& v)
        : configs(std::move(v)) {}

    static group_configs create(std::string_view json_str);
    std::string to_string() const;

    group_config get_config(std::size_t id) const {
        auto it = std::ranges::find_if(
            configs, [id](const auto& cfg) { return cfg.id == id; });
        if (it == configs.end()) {
            throw std::invalid_argument("Invalid id: " + std::to_string(id));
        }
        return *it;
    }
};

} // namespace uh::cluster::storage
