#include "service_registry.h"

#include "common/etcd/namespace.h"
#include "common/utils/common.h"
#include "common/utils/host_utils.h"

using namespace boost::asio;

namespace uh::cluster {

service_registry::service_registry(uh::cluster::role role, std::size_t index,
                                   etcd_manager& etcd)
    : m_service_role(role),
      m_id(index),
      m_etcd(etcd) {}

service_registry::~service_registry() {
    const std::string announced_key_base =
        get_announced_path(m_service_role, m_id);
    const std::string attribute_key_base =
        get_attributes_path(m_service_role, m_id);

    m_etcd.rmdir(announced_key_base);
    m_etcd.rmdir(attribute_key_base);
}

[[nodiscard]] std::string service_registry::get_service_name() const {
    return get_service_string(m_service_role) + "/" + std::to_string(m_id);
}

void service_registry::register_service(const server_config& config) {
    const std::string announced_key_base =
        get_announced_path(m_service_role, m_id);
    const std::string attribute_key_base =
        get_attributes_path(m_service_role, m_id);

    std::map<std::string, std::string> kv_pairs = {
        {attribute_key_base +
             get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST),
         get_host()},
        {attribute_key_base +
             get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT),
         std::to_string(config.port)},
        {attribute_key_base +
             get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PID),
         std::to_string(getpid())},
        {announced_key_base, {}},
    };

    for (auto& [k, v] : kv_pairs) {
        m_etcd.put(k, v);
    }
}

} // namespace uh::cluster
