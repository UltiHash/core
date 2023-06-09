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
#include <mutex>

#include "storage/backends/smart_backend/storage_types/fixed_managed_storage.h"
#include "storage/backends/smart_backend/storage_types/growing_managed_storage.h"
#include "storage/backends/smart_backend/storage_types/growing_plain_storage.h"

namespace uh::dbn::storage::smart {

class persisted_robinhood_hashmap {

public:

    explicit persisted_robinhood_hashmap (map_config map_conf);

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

    constexpr static size_t KEY_STORE_META_DATA_SIZE = sizeof (size_t);
    constexpr static size_t POOR_VALUE_SIZE = sizeof (uint8_t);
    constexpr static size_t VALUE_LENGTH_SIZE = sizeof (uint32_t);
    constexpr static size_t VALUE_PTR_SIZE = sizeof (uint64_t);

    const map_config m_map_conf;
    const size_t m_key_value_span_size;
    const size_t m_hash_element_size;
    std::vector <char> m_empty_key;
    std::shared_mutex m_mutex;
    growing_plain_storage m_key_store;
    growing_managed_storage m_value_store;
    std::atomic_ref <size_t> m_inserted_keys_size;

};

} // end namespace uh::dbn::storage::smart

#endif //CORE_PERSISTED_ROBINHOOD_HASHMAP_H
