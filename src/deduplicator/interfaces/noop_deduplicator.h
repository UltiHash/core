#pragma once

#include <common/service_interfaces/deduplicator_interface.h>
#include <storage/interfaces/global_data_view.h>

namespace uh::cluster {

class noop_deduplicator : public deduplicator_interface {
public:
    noop_deduplicator(sn::interface& storage);

    coro<dedupe_response> deduplicate(context& ctx,
                                      std::string_view data) override;

private:
    sn::interface& m_storage;
};

} // namespace uh::cluster
