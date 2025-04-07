#pragma once

#include "common/types/address.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "storage/global_data/global_data_view.h"

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
    explicit fragmentation();

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
    coro<void> flush_storage(global_data_view& gdv);

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
        uint128_t stored_pointer;
        size_t stored_size{};

        // fields only used for unstored fragments
        std::optional<fragment_set::hint_type> hint;
        address addr;
    };

    void flush_fragments_internal(fragment_set& set);
    void mark_as_uploaded();

    void compute_unstored_addresses();
    void merge_consecutive_unstored();

    unique_buffer<char> unstored_to_buffer();

    std::list<dd_fragment> m_frags;
    std::vector<std::size_t> m_offsets;
    std::size_t m_effective_size;
    std::size_t m_unstored_size;
    address m_buffer_address;
};

} // namespace uh::cluster
