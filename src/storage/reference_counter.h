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
     * Increments the reference counters of the stripes by the specified counts
     * in a single transaction.
     *
     * @param A vector of refcount_t containing the reference counters for the
     * specified stripes.
     * @param upstream A bool indicating if this call originates from am
     * upstream service. If false, trying to increment an untracked stripe will
     * start tracking it. If true see @return.
     * @return In case upstream is true, a vector of refcount_t for which no
     * prior reference count information was available.
     */
    std::vector<refcount_t> increment(const std::vector<refcount_t>& refcounts,
                                      bool upstream = true);

    /***
     * Decrements the reference counters of the stripes by the specified counts
     * in a single transaction. In case an untracked stripe is referenced, the
     * transaction is rolled back and a std::runtime exception is thrown.
     *
     * @param A vector of refcount_t containing the reference counters for the
     * specified stripes.
     * @return The number of bytes freed by this operation.
     */
    std::size_t decrement(const std::vector<refcount_t>& refcounts);

    /***
     * Retrieves the reference counters for the stripes specified by stripe_ids.
     *
     * @param stripe_ids
     * @return A vector of refcount_t containing the reference counters for the
     * specified stripes.
     */
    std::vector<refcount_t>
    get_refcounts(const std::vector<std::size_t>& stripe_ids);

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
