//
// Created by masi on 6/19/23.
//

#ifndef CORE_KEY_STORE_INTERFACE_H
#define CORE_KEY_STORE_INTERFACE_H

#include <optional>
#include <span>
#include <list>
#include <string_view>
#include <stdexcept>
#include <storage/backends/smart_backend/persistent_sets/set_interface.h>

namespace uh::dbn::storage::smart::key_stores {

struct map_result {
    std::optional<std::span<const char>> data;
    sets::index_type index;
};

class key_store_interface {
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
    virtual std::list<std::span<char>> list_keys (const std::span<char> &start_key, const std::span<char> &end_key, const std::span<std::string_view> &labels) {
        throw std::runtime_error("not implemented");
    }

    /**
     * removes the given key from the key store
     * @param key
     */
    virtual void remove (std::span<char> key) = 0;

    virtual ~key_store_interface () = default;
};

} // end namespace uh::dbn::storage::smart::key_stores

#endif //CORE_KEY_STORE_INTERFACE_H
