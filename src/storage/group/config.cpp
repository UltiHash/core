#include <storage/group/config.h>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace uh::cluster {

void from_json(const json& j, storage_group_config& config) {
    j.at("data_shards").get_to(config.data_shards);
    j.at("parity_shards").get_to(config.parity_shards);
    j.at("members").get_to(config.members);
}

void to_json(json& j, const storage_group_config& config) {
    j = json{{"data_shards", config.data_shards},
             {"parity_shards", config.parity_shards},
             {"members", config.members}};
}

storage_group_config storage_group_config::create(std::string_view json_str) {
    json j = json::parse(json_str);
    return j.get<storage_group_config>();
}

std::vector<storage_group_config>
storage_group_config::create_multiple(std::string_view json_str) {
    json j = json::parse(json_str);
    return j.get<std::vector<storage_group_config>>();
}

std::string storage_group_config::to_string() const {
    json j;
    to_json(j, *this);
    return j.dump();
}

} // namespace uh::cluster
