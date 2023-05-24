//
// Created by masi on 5/23/23.
//

#ifndef CORE_ROBIN_HOOD_HASHMAP_H
#define CORE_ROBIN_HOOD_HASHMAP_H

#include <filesystem>
#include <span>
#include <optional>

#include <boost/thread.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "mmap_storage.h"

namespace uh::dbn::storage::smart {

class robin_hood_hashmap {

public:
    robin_hood_hashmap (std::filesystem::path key_file, mmap_storage& value_store);

    /**
     *
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

    [[nodiscard]] inline size_t hash_index (const std::span <char>& key) const noexcept;

    void increment_inserted_key_size ();

    constexpr static size_t MAP_INIT_KEY_FILE_SIZE = 1024ul;

    constexpr static double KEY_STORE_LOAD_FACTOR = 0.9;

    constexpr static size_t KEY_STORE_META_DATA_SIZE = sizeof (size_t);

    constexpr static size_t POOR_VALUE_SIZE = sizeof (uint8_t);
    constexpr static size_t KEY_SIZE = 64;
    constexpr static size_t VALUE_LENGTH_SIZE = sizeof (uint32_t);
    constexpr static size_t VALUE_PTR_SIZE = sizeof (uint64_t);
    constexpr static size_t KEY_VALUE_SPAN_SIZE = KEY_SIZE + VALUE_PTR_SIZE + VALUE_LENGTH_SIZE;
    constexpr static size_t HASH_ELEMENT_SIZE = POOR_VALUE_SIZE + KEY_VALUE_SPAN_SIZE;

    size_t m_key_file_size = MAP_INIT_KEY_FILE_SIZE;
    
    std::filesystem::path m_key_file;
    std::filesystem::path m_value_file;

    char m_empty_spot [KEY_SIZE] {};

    boost::shared_mutex m_mutex;

    char* m_key_store;
    mmap_storage& m_value_store;
    std::atomic_ref <size_t> m_inserted_keys_size;
};

} // end namespace uh::dbn::storage::smart

#endif //CORE_ROBIN_HOOD_HASHMAP_H
