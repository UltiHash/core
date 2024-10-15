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
    explicit fragmentation(dedupe_logger& dd_logger);

    /**
     * Push a new fragment that was uploaded before.
     */
    void push_stored(uint128_t pointer, size_t size, std::string_view data,
                     bool header);

    /**
     * Push a new unstored fragment.
     */
    void push_unstored(std::string_view data, bool header,
                       std::optional<fragment_set::hint_type>&& hint);

    /**
     * Convert all unstored fragments to stored fragments.
     */
    void flush_fragment_set(fragment_set& set);

    /**
     * Writes all unstored fragments to downstream storage.
     */
    coro<void> flush_storage(context& ctx, global_data_view& gdv);


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
    enum fragment_type { STORED, UNSTORED };

    struct dd_fragment {
        // mandatory fields for both stored and unstored fragments
        fragment_type type;
        std::string_view data;
        bool header = false;

        // fields only used for stored fragments
        uint128_t pointer;
        size_t size{};

        // fields only used for unstored fragments
        std::optional<fragment_set::hint_type> hint;
        address addr;
    };

    void flush_fragments_internal(fragment_set& set);
    coro<void> link_unstored_fragments(context& ctx, global_data_view& gdv);
    void mark_as_uploaded();

    void compute_unstored_addresses();

    unique_buffer<char> unstored_to_buffer();

    dedupe_logger& m_dedupe_logger;
    std::list<dd_fragment> m_frags;
    std::size_t m_effective_size;
    std::size_t m_unstored_size;
    address m_buffer_address;
};

} // namespace uh::cluster

#endif
