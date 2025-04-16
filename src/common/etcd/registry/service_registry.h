#pragma once

#include "common/etcd/utils.h"
#include "common/network/server.h"
#include <string>

namespace uh::cluster {

class service_registry {

public:
    service_registry(uh::cluster::role role, std::size_t index,
                     etcd_manager& etcd);
    virtual ~service_registry();

    [[nodiscard]] std::string get_service_name() const;

    virtual void register_service(const server_config& config);

protected:
    etcd_manager& m_etcd;
    const size_t m_id;

private:
    static constexpr std::size_t m_etcd_default_ttl = 30;
    const role m_service_role;
};

} // namespace uh::cluster
