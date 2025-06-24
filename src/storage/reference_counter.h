#pragma once

#include <filesystem>

#include "common/types/address.h"
#include <deque>
#include <functional>
#include <lmdbxx/lmdb++.h>
#include <unordered_set>

namespace uh::cluster {
class reference_counter {
public:
    enum operation { INCREMENT, DECREMENT };

    struct refcount_cmd {
        operation op;
        std::size_t offset;
        std::size_t size;
    };

    reference_counter(const std::filesystem::path& root, std::size_t page_size,
                      const std::function<std::size_t(std::size_t offset,
                                                      std::size_t size)>& cb);
    /***
     * Executes operations provided in the cmd_queue in a single transaction.
     * In case an enqueued decrement operation attempts to decrement an
     * untracked page, the transaction is rolled back and a std::runtime
     * exception is thrown. This call is only intended to be used by the
     * data store and should not be exported to be used by upstream services.
     * @return Disk space reclaimed in the context of this call
     */
    void execute(std::deque<refcount_cmd>& cmd_queue);

    /***
     * Increments the page reference counter for the regions specified by addr
     * in a single transaction. This call can safely be exported to be used by
     * upstream services.
     *
     * @param address: storage locations that are to be referenced.
     * @param count: number of references to register
     * @return An address containing all fragments in addr pointing to
     * untracked pages.
     */
    address increment(const address& addr, const std::size_t count = 1);

    /***
     * Decrements the page reference counter to data regions specified in addr
     * in a single transaction. In case addr points to an untracked page, the
     * transaction is rolled back and a std::runtime exception is thrown.
     * This call can safely be exported to be used by upstream services.
     * @param address: storage locations that are to be unreferenced.
     * @param count: number of references to deregister
     * @return number of bytes freed in response to removing references.
     */
    std::size_t decrement(const address& addr, const std::size_t count = 1);

private:
    lmdb::env m_env;
    std::size_t m_page_size;
    std::function<std::size_t(std::size_t offset, std::size_t size)> m_cb;
    uint32_t m_storage_id;

    bool increment(std::size_t page_id, std::size_t count, bool upstream,
                   lmdb::txn& txn, lmdb::dbi& dbi);
    void decrement(std::size_t page_id, std::size_t count,
                   std::unordered_set<std::size_t>& marked_for_deletion,
                   lmdb::txn& txn, lmdb::dbi& dbi);
    std::size_t free_storage(std::unordered_set<std::size_t>& pages_to_free);
    std::pair<std::size_t, std::size_t> get_page_range(std::size_t offset,
                                                       std::size_t size) const;
};
} // namespace uh::cluster
