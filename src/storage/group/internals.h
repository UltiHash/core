#pragma once

#include "impl/prefix.h"

#include <common/etcd/namespace.h>
#include <common/etcd/service.h>
#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>
#include <storage/group/storage_state_manager.h>

namespace uh::cluster::storage {

/*
 * etcd key interfaces, which doesn't need to remove key on destruction
 */
struct group_initialized_manager {
    static void put_persistant(etcd_manager& etcd, std::size_t group_id,
                               bool value) {
        etcd.put_persistant(get_prefix(group_id).group_initialized,
                            serialize(value));
    }

    // static auto get(etcd_manager& etcd, std::size_t group_id) {
    //     return deserialize<bool>(
    //         etcd.get(get_prefix(group_id).group_initialized));
    // };
};

struct storage_assignment_triggers_manager {
    static void put(etcd_manager& etcd, std::size_t group_id, bool value) {
        etcd.put(get_prefix(group_id).storage_assignment_trigger,
                 serialize(value));
    }

    // static auto get(etcd_manager& etcd, std::size_t group_id, ) {
    //     return deserialize<bool>(
    //         etcd.get(get_prefix(group_id).storage_assignment_trigger));
    // };
};

} // namespace uh::cluster::storage
