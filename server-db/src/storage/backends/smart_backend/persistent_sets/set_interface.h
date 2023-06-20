//
// Created by masi on 6/8/23.
//

#ifndef CORE_SET_INTERFACE_H
#define CORE_SET_INTERFACE_H

#include <string_view>
#include <optional>

namespace uh::dbn::storage::smart::sets {

struct position_info {
    std::optional <std::pair <uint64_t, std::string_view>> lower;
    std::optional <std::pair <uint64_t, std::string_view>> match;
    std::optional <std::pair <uint64_t, std::string_view>> upper;
    uint64_t hint {};
    int comp {};
};

extern position_info null_position;

class set_interface {
public:

    /**
     * Inserts the offset_pointer (persistent) to the data into the set and uses pos as hint where to place the fragment
     * @param data to be removed
     * @param data_offset offset of the data in the data store
     * @param pos position hint, some implementations may require this and rely on hint
     * @return
     */
    position_info push_back_pointer (const std::string_view& data, uint64_t data_offset, const position_info& pos = null_position) {
        return do_push_back_pointer (data, data_offset, pos);
    }

    /**
     * Find the given data or similar data (larges prefix) and return the position info.
     * @param data data to be searched
     * @param pos position hint for search
     * @return position info of the found index/similar indices
     */
    [[nodiscard]] position_info find (const std::string_view& data, const position_info& pos = null_position) const {
        return do_find(data, pos);
    };

    /**
     * Removes the fragment from the set with position hint to find the fragment.
     * @param data index to be removed
     * @param pos position hint
     */
    void remove (std::string_view& data, const position_info& pos = null_position) {
        return do_remove (data, pos);
    }

    /**
     * Synchronises the fragment at the given position to disk
     * @param pos position of the fragment
     */
    void sync (const position_info& pos) {
        return do_sync (pos);
    }

    virtual ~set_interface() = default;
protected:

    virtual position_info do_push_back_pointer (const std::string_view& data, uint64_t data_offset, const position_info& pos) = 0;

    [[nodiscard]] virtual position_info do_find (const std::string_view& data, const position_info& pos) const = 0;

    virtual void do_remove (std::string_view& data, const position_info& pos) = 0;

    virtual void do_sync (const position_info& pos) = 0;

};

} // end namespace uh::dbn::storage::smart::sets
#endif //CORE_SET_INTERFACE_H
