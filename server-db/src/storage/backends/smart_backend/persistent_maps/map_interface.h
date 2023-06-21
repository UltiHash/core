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
#include "storage/backends/common.h"

namespace uh::dbn::storage::smart::maps {

class map_interface {
public:

    /**
     * Inserts the given key value in the hash map.
     * @param key
     * @param value
     */
    virtual void insert (std::span<char> key, std::span<char> value, const index_type& index) = 0;

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
