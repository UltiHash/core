#include "service_registry.h"

#include "common/etcd/namespace.h"
#include "common/utils/common.h"
#include "common/utils/host_utils.h"

using namespace boost::asio;

namespace uh::cluster {

service_registry::service_registry(uh::cluster::role role, std::size_t index,
                                   etcd_manager& etcd)
    : m_etcd(etcd),
      m_id(index),
      m_service_role(role) {}

service_registry::~service_registry() {
    std::string announced_key_base = get_announced_path(m_service_role, m_id);
    std::string attribute_key_base = get_attributes_path(m_service_role, m_id);

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
        {attribute_key_base + get_etcd_service_attribute_string(ENDPOINT_HOST),
         get_host()},
        {attribute_key_base + get_etcd_service_attribute_string(ENDPOINT_PORT),
         std::to_string(config.port)},
        {announced_key_base, {}},
    };

    for (auto& [k, v] : kv_pairs) {
        m_etcd.put(k, v);
    }
}

} // namespace uh::cluster
