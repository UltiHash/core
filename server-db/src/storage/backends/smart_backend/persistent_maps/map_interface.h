//
// Created by masi on 6/19/23.
//

#ifndef CORE_MAP_INTERFACE_H
#define CORE_MAP_INTERFACE_H

#include <optional>
#include <span>
#include <list>
#include <string_view>
#include <stdexcept>

#include <storage/backends/smart_backend/persistent_sets/set_interface.h>

namespace uh::dbn::storage::smart::maps {

struct map_key_value {
    std::span <const char> key;
    std::span <const char> value;
    uint64_t offset {};
    uint64_t index_offset {};

    map_key_value (std::span <char> key_, std::span <char> value_):
            key (key_), value (value_) {}

    explicit map_key_value (const sets::set_data& set_data_):
            offset (set_data_.data_offset),
            index_offset (set_data_.index_offset) {
        const auto key_size = *reinterpret_cast <const uint16_t*> (set_data_.data.data ());
        key = {set_data_.data.data() + sizeof (key_size), key_size};
        value = {set_data_.data.data() + sizeof (key_size) + key_size, set_data_.data.size() - sizeof (key_size) - key_size};
    }
};


struct map_result {
    std::optional <map_key_value> lower;
    std::optional <map_key_value> match;
    std::optional <map_key_value> upper;
    sets::index_type index;

    map_result () = default;

    map_result (std::span <char> key, std::span <char> value):
            match ({key, value}) {}

    explicit map_result (const sets::set_result& set_res) {
        if (set_res.lower.has_value()) {
            lower = map_key_value {set_res.lower.value()};
        }
        if (set_res.match.has_value()) {
            match = map_key_value {set_res.match.value()};
        }
        if (set_res.upper.has_value()) {
            upper = map_key_value {set_res.upper.value()};
        }
        index = set_res.index;
    }

private:

};

class map_interface {
public:

    /**
     * Inserts the given key value in the hash map.
     * @param key
     * @param value
     */
    virtual void insert (std::span<char> key, std::span<char> value, const sets::index_type& index) = 0;

    /**
     * returns the fragments offset and sizes
     * @param key
     * @return
     */
    virtual map_result get (std::span<char> key) = 0;

    /**
     * Gives back the list of keys in the range of start_key to end_key with the given labels
     */
    virtual std::list<map_key_value> get_range (const std::span<char> &start_key, const std::span<char> &end_key) {
        throw std::runtime_error("not implemented");
    }

    /**
     * removes the given key from the key store
     * @param key
     */
    virtual void remove (std::span<char> key) = 0;

    virtual ~map_interface () = default;
};

} // end namespace uh::dbn::storage::smart::key_stores

#endif //CORE_MAP_INTERFACE_H
