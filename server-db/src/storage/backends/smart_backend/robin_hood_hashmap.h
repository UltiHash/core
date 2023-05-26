//
// Created by masi on 5/23/23.
//

#ifndef CORE_ROBIN_HOOD_HASHMAP_H
#define CORE_ROBIN_HOOD_HASHMAP_H

#include <filesystem>
#include <span>
#include <optional>
#include <atomic>

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "mmap_storage.h"
#include "growing_mmap_storage.h"

namespace uh::dbn::storage::smart {

class robin_hood_hashmap {

public:
    robin_hood_hashmap (const size_t key_size, std::filesystem::path key_file, std::filesystem::path value_file);

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

    ~robin_hood_hashmap ();

private:

    enum hit_stat: uint8_t {
        MATCH_HIT,
        EMPTY_HIT,
        SWAP_HIT,
        NON_HIT,
    };

    [[nodiscard]] inline size_t hash_index (const std::span <char>& key) const noexcept;

    void extend_key_store ();

    void rehash(size_t old_file_size, size_t new_file_size);

    std::tuple <robin_hood_hashmap::hit_stat, uint8_t, char*> try_place_key (std::span <char> key);
    std::tuple <robin_hood_hashmap::hit_stat, uint8_t, char*> insert_key (std::span <char> key);

    constexpr static size_t MAP_INIT_KEY_FILE_SIZE = 1024ul;
    constexpr static size_t MIN_VALUE_FILE_SIZE = 1024ul*1024ul;
    constexpr static size_t MAX_VALUE_FILE_SIZE = 16ul*1024ul*1024ul;
    constexpr static double KEY_STORE_LOAD_FACTOR = 0.9;
    constexpr static size_t KEY_STORE_META_DATA_SIZE = sizeof (size_t);
    constexpr static size_t POOR_VALUE_SIZE = sizeof (uint8_t);
    constexpr static size_t VALUE_LENGTH_SIZE = sizeof (uint32_t);
    constexpr static size_t VALUE_PTR_SIZE = sizeof (uint64_t);

    const size_t m_key_size;
    const size_t m_key_value_span_size;
    const size_t m_hash_element_size;
    std::vector <char> m_empty_key;
    size_t m_key_file_size = MAP_INIT_KEY_FILE_SIZE;
    std::filesystem::path m_key_file;
    std::filesystem::path m_value_file;
    std::shared_mutex m_mutex;
    char* m_key_store;
    growing_mmap_storage m_value_store;
    std::atomic_ref <size_t> m_inserted_keys_size;

};

} // end namespace uh::dbn::storage::smart

#endif //CORE_ROBIN_HOOD_HASHMAP_H
