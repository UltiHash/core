#ifndef CORE_DEDUPLICATOR_FRAGMENTATION_H
#define CORE_DEDUPLICATOR_FRAGMENTATION_H

#include "common/global_data/global_data_view.h"
#include "common/types/address.h"
#include "dedupe_logger.h"
#include "deduplicator/dedupe_set/fragment_set.h"

#include <list>
#include <variant>

namespace uh::cluster {

/**
 * Buffer fragments and send them to storage as bulk request.
 *
 * This class serves as a list of fragments, partially uploaded to the
 * downstream storage.
 */
class fragmentation {
public:
    struct unstored {
        std::string_view data;
        bool header;
        std::optional<fragment_set::hint_type> hint;

        bool uploaded = false;

        address addr;
    };

    struct stored {
        uint128_t pointer;
        size_t size{};
        std::string_view data;
        bool header = false;
    };

    fragmentation(dedupe_logger& dd_logger);

    /**
     * Push a new fragment that was uploaded before.
     */
    void push(stored&& st);

    /**
     * Push a new unstored fragment.
     */
    void push(unstored&& un);

    /**
     * Convert all unstored fragments to stored fragments. Uploads all frags to
     * downstream storage.
     */
    void flush_set(fragment_set& set);
    coro<void> flush_data(context& ctx, global_data_view& gdv);

    std::size_t effective_size() const;
    std::size_t unstored_size() const;

    /**
     * Return the address of the fragment list. This will throw if there are
     * still unstored fragments.
     */
    address make_address() const;

    address get_stored_fragments() const;

    void handle_rejected_fragments(const address& addr, fragment_set& set);

private:
    void flush_fragments(fragment_set& set);
    void mark_as_uploaded();

    void compute_unstored_addresses(const address& addr);

    unique_buffer<char> unstored_to_buffer();

    dedupe_logger& m_dedupe_logger;
    std::list<std::variant<stored, unstored>> m_frags;
    std::size_t m_effective_size;
    std::size_t m_unstored_size;
};

} // namespace uh::cluster

#endif
