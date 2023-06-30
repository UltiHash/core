//
// Created by masi on 6/8/23.
//

#ifndef CORE_FRAGMENT_SET_INTERFACE_H
#define CORE_FRAGMENT_SET_INTERFACE_H

#include <string_view>
#include <optional>

namespace uh::dbn::storage::smart::sets {

struct fragment {
    uint64_t m_data_offset;
    size_t m_size;
};

struct position_info {
    std::optional <std::pair <uint64_t, std::string_view>> lower;
    std::optional <std::pair <uint64_t, std::string_view>> match;
    std::optional <std::pair <uint64_t, std::string_view>> upper;
    uint64_t hint {};
    int comp {};
};

extern position_info null_position;

class fragment_set_interface {
public:

    /**
     * Inserts the fragment into the set and uses pos as hint where to place the fragment
     * @param frag fragment data
     * @param data_offset offset of the fragment data in the data store
     * @param pos position hint, some implementations may require this and rely on hint
     * @return
     */
    position_info insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos = null_position) {
        return do_insert_index(frag, data_offset, pos);
    }

    /**
     * Find the given fragment or similar fragments (larges prefix) and return the position info.
     * @param frag fragment to be searched
     * @param pos position hint for search
     * @return position info of the found fragment/similar fragments
     */
    [[nodiscard]] position_info find (const std::string_view& frag, const position_info& pos = null_position) const {
        return do_find(frag, pos);
    };

    /**
     * Removes the fragment from the set with position hint to find the fragment.
     * @param frag fragment to be removed
     * @param pos position hint
     */
    void remove (fragment& frag, const position_info& pos = null_position) {
        return do_remove (frag, pos);
    }

    /**
     * Synchronises the fragment at the given position to disk
     * @param pos position of the fragment
     */
    void sync (const position_info& pos) {
        return do_sync (pos);
    }

    virtual ~fragment_set_interface() = default;
protected:

    virtual position_info do_insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos) = 0;

    [[nodiscard]] virtual position_info do_find (const std::string_view& frag, const position_info& pos) const = 0;

    virtual void do_remove (fragment& frag, const position_info& pos) = 0;

    virtual void do_sync (const position_info& pos) = 0;

};

} // end namespace uh::dbn::storage::smart::sets
#endif //CORE_FRAGMENT_SET_INTERFACE_H
