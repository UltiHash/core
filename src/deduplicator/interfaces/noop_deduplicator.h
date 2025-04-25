#pragma once

#include <common/service_interfaces/deduplicator_interface.h>
#include <storage/global/data_view.h>

namespace uh::cluster {

class noop_deduplicator : public deduplicator_interface {
public:
    noop_deduplicator(storage::global::global_data_view& storage);

    coro<dedupe_response> deduplicate(std::string_view data) override;

private:
    storage::global::global_data_view& m_storage;
};

} // namespace uh::cluster
