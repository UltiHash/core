#include "service_registry.h"

#include <common/etcd/namespace.h>
#include <common/service_interfaces/hostport.h>
#include <common/utils/common.h>
#include <common/utils/host_utils.h>
#include <common/utils/strings.h>

using namespace boost::asio;

namespace uh::cluster {

service_registry::service_registry(etcd_manager& etcd, const std::string& key)
    : m_etcd(etcd),
      m_key{key} {}

service_registry::~service_registry() { m_etcd.rm(m_key); }

void service_registry::register_service(const server_config& config) {
    m_etcd.put(m_key, serialize(hostport(get_host(), config.port)));
}

} // namespace uh::cluster
