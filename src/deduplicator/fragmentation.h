#ifndef CORE_DEDUPLICATOR_FRAGMENTATION_H
#define CORE_DEDUPLICATOR_FRAGMENTATION_H

#include "common/global_data/global_data_view.h"
#include "common/types/address.h"
#include "fragment_set.h"

#include <list>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace uh::cluster {

class fragmentation {
public:
    struct unstored {
        std::string_view data;
        std::set<fragment_set_element>::const_iterator hint;
        bool uploaded = false;

        address addr;
    };

    fragmentation(global_data_view& storage);

    void push(const fragment& f);

    void push(unstored&& un);

    /**
     * Convert all unstored fragments to stored fragments. Uploads all frags to
     * downstream storage.
     */
    void flush();

    void mark_as_uploaded();

    std::size_t effective_size() const;
    std::size_t unstored_size() const;

    void flush_fragments(fragment_set& destination);

    address make_address() const;

private:
    void compute_unstored_addresses(const address& addr);

    std::vector<char> unstored_to_buffer();

    global_data_view& m_storage;
    std::list<std::variant<fragment, unstored>> m_frags;
    std::size_t m_effective_size;
    std::size_t m_unstored_size;
};

} // namespace uh::cluster

#endif
