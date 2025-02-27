#pragma once

#include "common/coroutines/worker_pool.h"
#include "common/telemetry/context.h"
#include "storage/interface.h"

#include "common/service_interfaces/deduplicator_interface.h"
#include "deduplicator/config.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "deduplicator/fragmentation.h"

namespace uh::cluster {

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(boost::asio::io_context& ioc, deduplicator_config config,
                       sn::interface& storage);

    coro<dedupe_response> deduplicate(context& ctx,
                                      std::string_view data) override;

private:
    coro<size_t> pursue_pointer(context& ctx, std::string_view& data,
                                uint128_t pointer, bool header,
                                fragmentation& fragments);

    deduplicator_config m_dedupe_conf;
    sn::interface& m_storage;
    dd::cache m_cache;
    fragment_set m_fragment_set;
    worker_pool m_dedupe_workers;
    constexpr static std::size_t pursue_size = 64 * KIBI_BYTE;
};
} // namespace uh::cluster
