#include "dedupe_array.h"
#include "remote_deduplicator.h"

namespace uh::cluster {

dedupe_array::dedupe_array(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t connections)
    : m_etcd(etcd),
      m_dedupe_maintainer(
          m_etcd, ns::root.deduplicator.hostports,
          service_factory<deduplicator_interface>(ioc, connections),
          {m_dedupe_load_balancer}) {}

coro<dedupe_response> dedupe_array::deduplicate(std::string_view data) {
    return m_dedupe_load_balancer.get().second->deduplicate(data);
}

} // namespace uh::cluster
