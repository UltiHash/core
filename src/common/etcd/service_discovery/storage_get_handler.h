#pragma once
#include "storage/interfaces/distributed.h"

namespace uh::cluster {

struct storage_get_handler {

    virtual std::shared_ptr<distributed_storage>
    get(const uint128_t& pointer) = 0;

    virtual std::shared_ptr<distributed_storage> get(std::size_t id) = 0;

    virtual bool contains(std::size_t id) = 0;

    virtual std::vector<std::shared_ptr<distributed_storage>>
    get_services() = 0;

    virtual ~storage_get_handler() = default;
};

} // end namespace uh::cluster
