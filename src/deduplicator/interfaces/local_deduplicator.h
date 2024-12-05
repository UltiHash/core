#ifndef UH_CLUSTER_LOCAL_DEDUPLICATOR_H
#define UH_CLUSTER_LOCAL_DEDUPLICATOR_H

#include "common/coroutines/worker_pool.h"
#include "common/global_data/global_data_view.h"
#include "common/telemetry/context.h"

#include "common/service_interfaces/deduplicator_interface.h"
#include "deduplicator/dedupe_logger.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "deduplicator/fragmentation.h"

namespace uh::cluster {

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(deduplicator_config config, global_data_view& storage);

    coro<dedupe_response> deduplicate(context& ctx,
                                      const std::string_view& data) override;

private:
    coro<size_t> pursue_pointer(context& ctx, std::string_view& data,
                                uint128_t pointer, bool header,
                                fragmentation& fragments);

    coro<dedupe_response> deduplicate_data(context& ctx, std::string_view data);

    deduplicator_config m_dedupe_conf;
    fragment_set m_fragment_set;
    global_data_view& m_storage;
    worker_pool m_dedupe_workers;
    dedupe_logger m_dedupe_logger;
    constexpr static std::size_t pursue_size = 64 * KIBI_BYTE;
    constexpr static std::size_t pieces_count = 2;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_LOCAL_DEDUPLICATOR_H
