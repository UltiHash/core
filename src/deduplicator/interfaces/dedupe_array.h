#pragma once

#include <common/etcd/service_discovery/roundrobin_load_balancer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/deduplicator_interface.h>

namespace uh::cluster {

class dedupe_array : public deduplicator_interface {
public:
    dedupe_array(boost::asio::io_context& ioc, etcd_manager& etcd,
                 std::size_t connections);
    ~dedupe_array() override;

    coro<dedupe_response> deduplicate(context& ctx,
                                      std::string_view data) override;

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    service_maintainer<deduplicator_interface> m_dedupe_maintainer;
    roundrobin_load_balancer<deduplicator_interface> m_dedupe_load_balancer;
};

} // namespace uh::cluster
