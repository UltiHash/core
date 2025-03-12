#pragma once
#include "common/service_interfaces/storage_interface.h"

namespace uh::cluster {

class storage_get_handler {

public:
    virtual std::shared_ptr<storage_interface>
    get(const uint128_t& pointer) = 0;

    virtual std::shared_ptr<storage_interface> get(std::size_t id) = 0;

    virtual bool contains(std::size_t id) = 0;

    virtual std::vector<std::shared_ptr<storage_interface>> get_services() = 0;

    virtual ~storage_get_handler() = default;
};

} // end namespace uh::cluster
