#include "service_registry.h"

#include "common/utils/common.h"
#include "etcd/v3/Transaction.hpp"
#include "namespace.h"
#include <boost/asio.hpp>

using namespace boost::asio;

namespace uh::cluster {

namespace {

bool is_valid_ip(const std::string& ip) {
    try {
        ip::address address = ip::address::from_string(ip);
        return address.is_v4() || address.is_v6();
    } catch (const std::exception& e) {
        return false;
    }
}

std::string get_host() {
    const char* var_value = std::getenv(ENV_CFG_ENDPOINT_HOST);
    if (var_value == nullptr) {
        return ip::host_name();
    } else {
        if (is_valid_ip(var_value))
            return {var_value};
        else
            throw std::invalid_argument(
                "the environmental variable " +
                std::string(ENV_CFG_ENDPOINT_HOST) +
                " does not contain a valid IPv4 or IPv6 address: '" +
                std::string(var_value) + "'");
    }
}

} // namespace

service_registry::service_registry(uh::cluster::role role, std::size_t index,
                                   etcd::SyncClient& etcd_client)
    : m_service_name(get_service_string(role) + "/" + std::to_string(index)),
      m_etcd_client(etcd_client) {}

[[nodiscard]] const std::string& service_registry::get_service_name() const {
    return m_service_name;
}

service_registry::registration::registration(
    etcd::SyncClient& client,
    const std::map<std::string, std::string>& kv_pairs, std::size_t ttl)
    : m_client(client),
      m_lease(m_client.leasegrant(ttl).value().lease()),
      m_keepalive(m_client, ttl, m_lease) {
    etcdv3::Transaction txn;
    for (const auto& pair : kv_pairs)
        txn.add_success_put(pair.first, pair.second, m_lease);
    m_client.txn(txn);
}

service_registry::registration::~registration() {
    m_client.leaserevoke(m_lease);
}

std::unique_ptr<service_registry::registration>
service_registry::register_service(const server_config& config) {

    const std::string announced_key_base =
        etcd_services_announced_key_prefix + m_service_name;

    const std::string key_base =
        etcd_services_attributes_key_prefix + m_service_name + "/";

    const std::map<std::string, std::string> kv_pairs = {
        {key_base + get_config_string(uh::cluster::CFG_ENDPOINT_HOST),
         get_host()},
        {key_base + get_config_string(uh::cluster::CFG_ENDPOINT_PORT),
         std::to_string(config.port)},
        {announced_key_base, {}},
    };

    return std::make_unique<registration>(m_etcd_client, kv_pairs,
                                          m_etcd_default_ttl);
}

} // namespace uh::cluster
