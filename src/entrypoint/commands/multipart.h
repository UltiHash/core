#ifndef ENTRYPOINT_HTTP_MULTIPART_H
#define ENTRYPOINT_HTTP_MULTIPART_H

#include "command.h"
#include "common/etcd/service_discovery/roundrobin_load_balancer.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class multipart : public command {
public:
    explicit multipart(roundrobin_load_balancer<deduplicator_interface>&,
                       multipart_state&);

    static bool can_handle(const http_request& req);

    coro<void> validate(const http_request& req) override;

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    roundrobin_load_balancer<deduplicator_interface>& m_dedupe_services;
    multipart_state& m_uploads;
};

} // namespace uh::cluster

#endif
