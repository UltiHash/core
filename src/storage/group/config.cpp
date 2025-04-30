#include <storage/group/config.h>

using nlohmann::ordered_json;

namespace uh::cluster::storage {

void from_json(const ordered_json& j, group_config& config) {
    j.at("id").get_to(config.id);
    auto type_str = j.at("type").get<std::string>();
    auto type_enum = magic_enum::enum_cast<group_config::type_t>(type_str);
    if (!type_enum) {
        throw std::invalid_argument("Invalid type: " + type_str);
    }
    config.type = *type_enum;
    j.at("storages").get_to(config.storages);

    if (type_enum == group_config::type_t::ERASURE_CODING) {
        j.at("data_shards").get_to(config.data_shards);
        j.at("parity_shards").get_to(config.parity_shards);
    }
}

void to_json(ordered_json& j, const group_config& config) {
    j = ordered_json{{"id", config.id},
                     {"type", std::string(magic_enum::enum_name(config.type))},
                     {"storages", config.storages}};

    if (config.type == group_config::type_t::ERASURE_CODING) {
        j["data_shards"] = config.data_shards;
        j["parity_shards"] = config.parity_shards;
    }
}

group_config group_config::create(std::string_view json_str) {
    ordered_json j = ordered_json::parse(json_str);
    return j.get<group_config>();
}

std::string group_config::to_string() const {
    ordered_json j;
    to_json(j, *this);
    return j.dump();
}

group_configs group_configs::create(std::string_view json_str) {
    ordered_json j = ordered_json::parse(json_str);
    return group_configs(j.get<std::vector<group_config>>());
}

std::string group_configs::to_string() const {
    ordered_json j;
    to_json(j, configs);
    return j.dump();
}

} // namespace uh::cluster::storage
