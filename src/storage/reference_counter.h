#pragma once

#include <filesystem>

#include "common/types/common_types.h"
#include <deque>
#include <functional>
#include <lmdbxx/lmdb++.h>
#include <unordered_set>

namespace uh::cluster {
class reference_counter {
public:
    reference_counter(const std::filesystem::path& root,
                      std::size_t stripe_size,
                      const std::function<std::size_t(std::size_t offset,
                                                      std::size_t size)>& cb);
    /***
     * Increments the page reference counter for the regions specified by addr
     * in a single transaction. This call can safely be exported to be used by
     * upstream services.
     *
     * @param addr
     * @return An address containing all fragments in addr pointing to
     * untracked pages.
     */
    std::vector<refcount_t> increment(const std::vector<refcount_t>& refcounts,
                                      bool upstream = true);

    /***
     * Decrements the page reference counter to data regions specified in addr
     * in a single transaction. In case addr points to an untracked page, the
     * transaction is rolled back and a std::runtime exception is thrown.
     * This call can safely be exported to be used by upstream services.
     * @param addr
     * @return Disk space reclaimed in the context of this call
     */
    std::size_t decrement(const std::vector<refcount_t>& refcounts);

private:
    lmdb::env m_env;
    std::size_t m_stripe_size;
    std::function<std::size_t(std::size_t offset, std::size_t size)> m_cb;

    static bool increment(std::size_t stripe_id, std::size_t count,
                          bool upstream, lmdb::txn& txn, lmdb::dbi& dbi);
    static bool decrement(std::size_t stripe_id, std::size_t count,
                          lmdb::txn& txn, lmdb::dbi& dbi);
    std::size_t free_stripes(std::vector<std::size_t>& stripes_to_free);
};
} // namespace uh::cluster
