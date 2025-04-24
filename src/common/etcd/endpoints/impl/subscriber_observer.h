#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster {

struct subscriber_observer {
    virtual void on_watch(etcd_manager::response resp) = 0;
    virtual ~subscriber_observer() = default;
};

} // namespace uh::cluster
