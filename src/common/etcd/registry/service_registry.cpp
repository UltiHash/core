#include "service_registry.h"

#include "common/utils/common.h"
#include "common/utils/host_utils.h"

using namespace boost::asio;

namespace uh::cluster {

service_registry::service_registry(uh::cluster::role role, std::size_t index,
                                   etcd::SyncClient& etcd_client)
    : m_service_role(role),
      m_id(index),
      m_etcd_client(etcd_client) {}

[[nodiscard]] std::string service_registry::get_service_name() const {
    return get_service_string(m_service_role) + "/" + std::to_string(m_id);
}

service_registry::registration::registration(
    etcd::SyncClient& client, role r, size_t id,
    const std::map<std::string, std::string>& kv_pairs, std::size_t ttl)
    : m_client(client),
      m_lease(m_client.leasegrant(ttl).value().lease()),
      m_keepalive(m_client, ttl, m_lease),
      m_service_role(r),
      m_id(id),
      m_monitor_thread([this] {
          while (true) {

              std::unique_lock<std::mutex> lock(m_attributes_mutex);
              if (m_stop) {
                  return;
              }
              if (m_cv.wait_for(lock, std::chrono::seconds(m_etcd_default_ttl),
                                [this] { return m_stop; })) {
                  return;
              }

              for (auto& kv : m_monitored_attributes) {
                  m_client.put(kv.first, kv.second(), m_lease);
              }
          }
      }) {
    for (const auto& pair : kv_pairs)
        m_client.add(pair.first, pair.second, m_lease);
}

void service_registry::registration::monitor(
    etcd_service_attributes key, const std::function<std::string()>& func) {
    auto key_base =
        get_attributes_path(m_service_role, m_id) + get_etcd_key_string(key);
    std::lock_guard<std::mutex> lock(m_attributes_mutex);
    m_monitored_attributes.emplace(key_base, func);
    m_cv.notify_all();
}

service_registry::registration::~registration() {
    {
        std::unique_lock lk(m_attributes_mutex);
        m_stop = true;
    }
    m_cv.notify_all();

    m_monitor_thread.join();
    m_client.leaserevoke(m_lease);
    m_keepalive.Cancel();
}

std::unique_ptr<service_registry::registration>
service_registry::register_service(const server_config& config) {

    const std::string announced_key_base =
        get_announced_path(m_service_role, m_id);

    const std::string attribute_key_base =
        get_attributes_path(m_service_role, m_id);

    const std::map<std::string, std::string> kv_pairs = {
        {attribute_key_base + get_etcd_key_string(uh::cluster::ENDPOINT_HOST),
         get_host()},
        {attribute_key_base + get_etcd_key_string(uh::cluster::ENDPOINT_PORT),
         std::to_string(config.port)},
        {attribute_key_base + get_etcd_key_string(uh::cluster::ENDPOINT_PID),
         std::to_string(getpid())},
        {announced_key_base, {}},
    };

    return std::make_unique<registration>(m_etcd_client, m_service_role, m_id,
                                          kv_pairs, m_etcd_default_ttl);
}

} // namespace uh::cluster
