//
// Created by masi on 5/23/23.
//

#ifndef CORE_PERSISTED_ROBINHOOD_HASHMAP_H
#define CORE_PERSISTED_ROBINHOOD_HASHMAP_H

#include <filesystem>
#include <span>
#include <optional>
#include <atomic>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "fixed_managed_storage.h"
#include "growing_managed_storage.h"
#include "growing_plain_storage.h"

namespace uh::dbn::storage::smart {

class persisted_robinhood_hashmap {

public:
    persisted_robinhood_hashmap (const size_t key_size, std::filesystem::path key_file, std::filesystem::path value_file);

    /**
     * Inserts the given key value in the hash map.
     * @param key
     * @param value
     */
    void insert (std::span <char> key, std::span <char> value);

    /**
     * returns the fragments offset and sizes
     * @param key
     * @return
     */
    std::optional <std::span <char>> get (std::span <char> key);

    void remove (std::span <char> key);

    ~persisted_robinhood_hashmap ();

private:

    enum hit_enum: uint8_t {
        MATCH_HIT,
        EMPTY_HIT,
        SWAP_HIT,
    };

    struct insert_stat {
        char* element_pos = nullptr;
        hit_enum hit_stat;
        uint8_t dangling_poor_value {};
    };

    [[nodiscard]] inline size_t hash_index (const std::span <char>& key) const noexcept;

    bool rehash(growing_plain_storage& old_key_store);

    insert_stat try_place_key (std::span <char> key);
    insert_stat insert_key (std::span <char> key);
    inline bool need_rehash (const char* inserted_element) const;
    void extend_and_rehash ();

    constexpr static size_t MAP_INIT_KEY_FILE_SIZE = 1024ul;
    constexpr static size_t MIN_VALUE_FILE_SIZE = 1024ul*1024ul;
    constexpr static size_t MAX_VALUE_FILE_SIZE = 16ul*1024ul*1024ul;
    constexpr static double KEY_STORE_LOAD_FACTOR = 0.9;
    constexpr static size_t KEY_STORE_META_DATA_SIZE = sizeof (size_t);
    constexpr static size_t POOR_VALUE_SIZE = sizeof (uint8_t);
    constexpr static size_t VALUE_LENGTH_SIZE = sizeof (uint32_t);
    constexpr static size_t VALUE_PTR_SIZE = sizeof (uint64_t);
    constexpr static size_t MAX_EXTENSION_FACTOR = 32;

    const size_t m_key_size;
    const size_t m_key_value_span_size;
    const size_t m_hash_element_size;
    std::vector <char> m_empty_key;
    std::filesystem::path m_value_file;
    std::shared_mutex m_mutex;
    growing_plain_storage m_key_store;
    growing_managed_storage m_value_store;
    std::atomic_ref <size_t> m_inserted_keys_size;

};

} // end namespace uh::dbn::storage::smart

#endif //CORE_PERSISTED_ROBINHOOD_HASHMAP_H
