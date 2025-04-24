#pragma once

#include <common/coroutines/worker_pool.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <deduplicator/config.h>
#include <deduplicator/dedupe_set/fragment_set.h>
#include <deduplicator/fragmentation.h>
#include <storage/global/cache.h>
#include <storage/global/data_view.h>

namespace uh::cluster {

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(deduplicator_config config, storage::data_view& storage,
                       storage::global::cache& cache);

    coro<dedupe_response> deduplicate(std::string_view data) override;

private:
    coro<size_t> pursue_pointer(std::string_view& data, uint128_t pointer,
                                bool header, fragmentation& fragments);

    deduplicator_config m_dedupe_conf;
    storage::data_view& m_storage;
    storage::global::cache& m_cache;
    fragment_set m_fragment_set;
    worker_pool m_dedupe_workers;
    constexpr static std::size_t pursue_size = 64 * KIBI_BYTE;
};
} // namespace uh::cluster
