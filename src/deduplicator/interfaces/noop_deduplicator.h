#pragma once

#include <common/global_data/default_global_data_view.h>
#include <common/service_interfaces/deduplicator_interface.h>

namespace uh::cluster {

class noop_deduplicator : public deduplicator_interface {
public:
    noop_deduplicator(global_data_view& storage);

    coro<dedupe_response> deduplicate(context& ctx,
                                      std::string_view data) override;

private:
    global_data_view& m_storage;
};

} // namespace uh::cluster
