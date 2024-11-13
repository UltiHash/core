#ifndef UH_CLUSTER_REFERENCE_COUNTER_H
#define UH_CLUSTER_REFERENCE_COUNTER_H

#include <filesystem>

#include "common/types/address.h"
#include <functional>
#include <lmdbxx/lmdb++.h>

namespace uh::cluster {
class reference_counter {
public:
    reference_counter(const std::filesystem::path& root, std::size_t page_size,
                      const std::function<std::size_t(std::size_t offset,
                                                      std::size_t size)>& cb);
    /***
     * Decrements the page reference counter to data regions specified in addr
     * in a single transaction. In case addr points to an untracked page, the
     * transaction is rolled back and a std::runtime exception is thrown.
     * This call can safely be exported to be used by upstream services.
     * @param addr
     * @return Disk space reclaimed in the context of this call
     */
    std::size_t decrement(const address& addr);

    /***
     * Increments the page reference counter for the region specified by offset
     * and size in a single transaction. This call is only intended to be used
     * by the data store and may not be exposed to upstream services, as it
     * initializes page counters if none were available previously.
     * @param offset
     * @param size
     */
    void increment(std::size_t offset, std::size_t size);

    /***
     * Increments the page reference counter for the regions specified by addr
     * in a single transaction. This call can safely be exported to be used by
     * upstream services.
     *
     * @param addr
     * @return An address containing all fragments in addr pointing to
     * untracked pages.
     */
    address increment(const address& addr);

private:
    lmdb::env m_env;
    std::size_t m_page_size;
    std::function<std::size_t(std::size_t offset, std::size_t size)> m_cb;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_REFERENCE_COUNTER_H
