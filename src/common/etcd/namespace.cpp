#include <common/etcd/namespace.h>
#include <format>

namespace uh::cluster {

std::string get_storage_groups_assigned_storages_path(size_t group_id) {
    return std::format("{}{}/storages", etcd_storage_groups_key_prefix,
                       group_id);
}

std::string get_storage_groups_assigned_storages_path(size_t group_id,
                                                      size_t storage_id) {
    return std::format("{}{}/storages/{}", etcd_storage_groups_key_prefix,
                       group_id, storage_id);
}

std::string get_storage_group_initialized_flag_path(size_t group_id) {
    return std::format("{}{}/initialized", etcd_storage_groups_key_prefix,
                       group_id);
}

std::string get_storage_group_state_path(size_t group_id) {
    return std::format("{}state/{}", etcd_storage_groups_key_prefix, group_id);
}

std::string get_storage_group_config_path(size_t group_id) {
    return std::format("{}config/{}", etcd_storage_groups_key_prefix, group_id);
}

std::string get_storage_to_storage_group_map_path(size_t storage_id) {
    return std::format("{}assignments/{}", etcd_storage_groups_key_prefix,
                       storage_id);
}

} // namespace uh::cluster
