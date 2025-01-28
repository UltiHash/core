#include "dedupe_array.h"

namespace uh::cluster {

dedupe_array::dedupe_array(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t connections)
    : m_ioc(ioc),
      m_etcd(etcd),
      m_dedupe_maintainer(m_etcd, service_factory<deduplicator_interface>(
                                      m_ioc, connections, {})) {
    m_dedupe_maintainer.add_monitor(m_dedupe_load_balancer);
}

dedupe_array::~dedupe_array() {
    m_dedupe_maintainer.remove_monitor(m_dedupe_load_balancer);
}

coro<dedupe_response> dedupe_array::deduplicate(context& ctx,
                                                std::string_view data) {
    return m_dedupe_load_balancer.get()->deduplicate(ctx, data);
}

} // namespace uh::cluster
