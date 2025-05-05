#pragma once

#include <common/etcd/service_discovery/service_load_balancer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/deduplicator_interface.h>

namespace uh::cluster {

class dedupe_array : public deduplicator_interface {
public:
    dedupe_array(boost::asio::io_context& ioc, etcd_manager& etcd,
                 std::size_t connections);

    coro<dedupe_response> deduplicate(std::string_view data) override;

private:
    etcd_manager& m_etcd;
    service_load_balancer<deduplicator_interface> m_dedupe_load_balancer;
    service_maintainer<deduplicator_interface> m_dedupe_maintainer;
};

} // namespace uh::cluster
