#include <storage/group/config.h>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace uh::cluster::storage::group {

void from_json(const json& j, config& config) {
    j.at("data_shards").get_to(config.data_shards);
    j.at("parity_shards").get_to(config.parity_shards);
    j.at("members").get_to(config.members);
}

void to_json(json& j, const config& config) {
    j = json{{"data_shards", config.data_shards},
             {"parity_shards", config.parity_shards},
             {"members", config.members}};
}

config config::create(std::string_view json_str) {
    json j = json::parse(json_str);
    return j.get<config>();
}

std::vector<config> config::create_multiple(std::string_view json_str) {
    json j = json::parse(json_str);
    return j.get<std::vector<config>>();
}

std::string config::to_string() const {
    json j;
    to_json(j, *this);
    return j.dump();
}

} // namespace uh::cluster::storage::group
