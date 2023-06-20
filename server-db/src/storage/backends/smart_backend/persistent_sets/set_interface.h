//
// Created by masi on 6/8/23.
//

#ifndef CORE_SET_INTERFACE_H
#define CORE_SET_INTERFACE_H

#include <string_view>
#include <optional>

namespace uh::dbn::storage::smart::sets {

struct index_type {
    uint64_t position;
    int comp;
};

struct set_result {
    std::optional <std::pair <uint64_t, std::string_view>> lower;
    std::optional <std::pair <uint64_t, std::string_view>> match;
    std::optional <std::pair <uint64_t, std::string_view>> upper;
    index_type index;
};

extern index_type null_index;

class set_interface {
public:

    /**
     * Inserts the offset_pointer (persistent) to the data into the set and uses pos as hint where to place the fragment
     * @param data to be removed
     * @param data_offset offset of the data in the data store
     * @param pos position hint, some implementations may require this and rely on hint
     * @return
     */
    index_type add_pointer (const std::string_view& data, uint64_t data_offset, const index_type& pos = null_index) {
        return do_add_pointer (data, data_offset, pos);
    }

    /**
     * Find the given data or similar data (larges prefix) and return the position info.
     * @param data data to be searched
     * @param pos position hint for search
     * @return position info of the found index/similar indices
     */
    [[nodiscard]] set_result find (const std::string_view& data, const index_type& pos = null_index) const {
        return do_find(data, pos);
    };

    /**
     * Removes the fragment from the set with position hint to find the fragment.
     * @param data index to be removed
     * @param pos position hint
     */
    void remove (std::string_view& data, const index_type& pos = null_index) {
        return do_remove (data, pos);
    }

    /**
     * Synchronises the fragment at the given position to disk
     * @param pos position of the fragment
     */
    void sync (const index_type& pos) {
        return do_sync (pos);
    }

    virtual ~set_interface() = default;
protected:

    virtual index_type do_add_pointer (const std::string_view& data, uint64_t data_offset, const index_type& pos) = 0;

    [[nodiscard]] virtual set_result do_find (const std::string_view& data, const index_type& pos) const = 0;

    virtual void do_remove (std::string_view& data, const index_type& pos) = 0;

    virtual void do_sync (const index_type& pos) = 0;

};

} // end namespace uh::dbn::storage::smart::sets
#endif //CORE_SET_INTERFACE_H
