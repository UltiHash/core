#pragma once

#include <common/types/big_int.h>

namespace uh::cluster {

template <typename service> struct storage_get_handler {

    virtual std::shared_ptr<service> get(const uint128_t& pointer) = 0;

    virtual std::shared_ptr<service> get(std::size_t id) = 0;

    virtual bool contains(std::size_t id) = 0;

    virtual std::vector<std::shared_ptr<service>> get_services() = 0;

    virtual ~storage_get_handler() = default;
};

} // end namespace uh::cluster
