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
    position_info insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos = null_position) {
        return do_insert_index(frag, data_offset, pos);
    }

    [[nodiscard]] position_info find (const std::string_view& frag, const position_info& pos = null_position) const {
        return do_find(frag, pos);
    };

    void remove (fragment& frag, const position_info& pos = null_position) {
        return do_remove (frag, pos);
    }

    void sync (const position_info& pos = null_position) {
        return do_sync (pos);
    }

protected:

    virtual position_info do_insert_index (const std::string_view& frag, uint64_t data_offset, const position_info& pos) = 0;

    [[nodiscard]] virtual position_info do_find (const std::string_view& frag, const position_info& pos) const = 0;

    virtual void do_remove (fragment& frag, const position_info& pos) = 0;

    virtual void do_sync (const position_info& pos) = 0;

};

} // end namespace uh::dbn::storage::smart::sets
#endif //CORE_FRAGMENT_SET_INTERFACE_H
