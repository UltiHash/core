#include "service_registry.h"

#include <common/etcd/namespace.h>
#include <common/service_interfaces/hostport.h>
#include <common/utils/common.h>
#include <common/utils/host_utils.h>
#include <common/utils/strings.h>

using namespace boost::asio;

namespace uh::cluster {

service_registry::service_registry(etcd_manager& etcd, const std::string& key,
                                   uint16_t port)
    : m_etcd(etcd),
      m_key{key} {
    LOG_DEBUG() << "Registering service with port: " << port;
    m_etcd.put(m_key, serialize(hostport(get_host(), port)));
}

service_registry::~service_registry() { m_etcd.rm(m_key); }

} // namespace uh::cluster
