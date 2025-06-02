#pragma once

#include "common/etcd/utils.h"
#include "common/network/server.h"
#include <magic_enum/magic_enum.hpp>
#include <string>

namespace uh::cluster {

class service_registry {

public:
    service_registry(etcd_manager& etcd, const std::string& key, uint16_t port);
    ~service_registry();

protected:
    etcd_manager& m_etcd;

private:
    std::string m_key;
};

} // namespace uh::cluster
