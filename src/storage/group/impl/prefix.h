#pragma once

#include <common/etcd/namespace.h>

namespace uh::cluster::storage {

using prefix_t = ns::storage_groups_t::impl_t;

inline prefix_t get_prefix(size_t group_id) {
    return ns::root.storage_groups[group_id];
}

using offset_prefix_t = ns::subscriptable_key_t;

inline offset_prefix_t get_storage_offset_prefix(size_t group_id) {
    return ns::root.storage_groups.temporaries[group_id].storage_offsets;
}

} // namespace uh::cluster::storage
