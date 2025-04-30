#pragma once

#include "common/etcd/namespace.h"

namespace uh::cluster {

template <typename service_interface> class service_observer {

public:
    virtual void add_client(size_t, std::shared_ptr<service_interface>) = 0;
    virtual void remove_client(size_t) = 0;

    virtual ~service_observer() = default;
};
} // namespace uh::cluster
