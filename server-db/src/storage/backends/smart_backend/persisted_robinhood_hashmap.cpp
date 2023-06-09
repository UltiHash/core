//
// Created by masi on 5/24/23.
//
#include "persisted_robinhood_hashmap.h"

namespace uh::dbn::storage::smart {

persisted_robinhood_hashmap::persisted_robinhood_hashmap(map_config map_conf) :
        m_map_conf(std::move (map_conf)),
        m_key_value_span_size (m_map_conf.key_size + VALUE_PTR_SIZE + VALUE_LENGTH_SIZE),
        m_hash_element_size (POOR_VALUE_SIZE + m_key_value_span_size),
        m_empty_key (m_map_conf.key_size),
        m_key_store (growing_plain_storage (m_map_conf.hashtable_key_path, m_map_conf.map_key_file_init_size)),
        m_value_store (m_map_conf.hashtable_value_directory, m_map_conf.value_store_log_file, m_map_conf.map_values_minimum_file_size, m_map_conf.map_values_maximum_file_size),
        m_inserted_keys_size {*reinterpret_cast <size_t*> (m_key_store.get_storage())} {
    std::memset (m_empty_key.data(), 0, m_map_conf.key_size);
}


void persisted_robinhood_hashmap::insert(std::span<char> key, std::span<char> value) {

    std::unique_lock <std::shared_mutex> lock (m_mutex);

    auto res = insert_key (key);

    const std::uint32_t value_size = value.size_bytes();
    const auto offset_ptr = m_value_store.allocate(value.size_bytes());

    const auto key_store_value_index = res.element_pos + POOR_VALUE_SIZE + m_map_conf.key_size;
    std::memcpy (key_store_value_index, &offset_ptr.m_offset, VALUE_PTR_SIZE);
    std::memcpy (key_store_value_index + VALUE_PTR_SIZE, &value_size, VALUE_LENGTH_SIZE);

    if (need_rehash(res.element_pos)) {
        extend_and_rehash();
    }

    lock.unlock();

    const auto value_ptr = m_value_store.get_raw_ptr(offset_ptr.m_offset);
    std::memcpy (value_ptr, value.data(), value_size);

    sync_ptr (key_store_value_index, VALUE_PTR_SIZE + VALUE_LENGTH_SIZE);
    sync_ptr (value_ptr, value_size);
}

std::optional<std::span<char>> persisted_robinhood_hashmap::get(std::span<char> key) {
    auto index = hash_index (key);

    std::shared_lock <std::shared_mutex> lock (m_mutex);

    uint8_t expected_poor_value = 0;
    while (std::memcmp (m_key_store.get_storage() + index + POOR_VALUE_SIZE, key.data(), m_map_conf.key_size) != 0) {

        if (expected_poor_value > m_key_store.get_storage() [index]) {
            return std::nullopt;
        }

        expected_poor_value ++;
        index += m_hash_element_size;
    }

    const auto value_offset = *reinterpret_cast <uint64_t*> (m_key_store.get_storage() + index + POOR_VALUE_SIZE + m_map_conf.key_size);
    const auto value_size = *reinterpret_cast <uint32_t*> (m_key_store.get_storage() + index + POOR_VALUE_SIZE + m_map_conf.key_size + VALUE_PTR_SIZE);

    const auto value_ptr = m_value_store.get_raw_ptr(value_offset);
    return {std::span <char> (static_cast <char *> (value_ptr), value_size)};
}

persisted_robinhood_hashmap::~persisted_robinhood_hashmap() {
    m_value_store.sync();
}

size_t persisted_robinhood_hashmap::hash_index(const std::span<char> &key) const noexcept {
    size_t h = 0ul;
    for (int i = 0; i < 2 * sizeof (size_t); i += sizeof (size_t)) {
        h += *reinterpret_cast <size_t*> (key.data() + i);
    }
    auto pos = h % (m_key_store.get_size() - 2 * m_hash_element_size - KEY_STORE_META_DATA_SIZE);
    pos += m_hash_element_size - pos % m_hash_element_size;

    return pos + KEY_STORE_META_DATA_SIZE;
}

bool persisted_robinhood_hashmap::rehash(growing_plain_storage& old_key_store) {
    const auto old_key_store_end = reinterpret_cast <char*> (old_key_store.get_storage() + old_key_store.get_size());
    for (char* i = old_key_store.get_storage() + KEY_STORE_META_DATA_SIZE; i < old_key_store_end; i += m_hash_element_size) {
        if (std::memcmp (i + POOR_VALUE_SIZE, m_empty_key.data(), m_map_conf.key_size) != 0) {
            const auto res = insert_key({i + POOR_VALUE_SIZE, m_map_conf.key_size});
            std::memcpy (res.element_pos + POOR_VALUE_SIZE + m_map_conf.key_size,
                         i + POOR_VALUE_SIZE + m_map_conf.key_size,
                         VALUE_PTR_SIZE + VALUE_LENGTH_SIZE);
            if (need_rehash (res.element_pos)) {
                return false;
            }
        }
    }
    return true;
}

persisted_robinhood_hashmap::insert_stat persisted_robinhood_hashmap::try_place_key(std::span<char> key) {

    auto key_store_index = m_key_store.get_storage() + hash_index (key);
    std::uint8_t dangling_poor_value = 0;

    while (std::memcmp (key_store_index + POOR_VALUE_SIZE, key.data(), m_map_conf.key_size) != 0) {

        if (std::memcmp (key_store_index + POOR_VALUE_SIZE, m_empty_key.data(), m_map_conf.key_size) == 0) {

            m_inserted_keys_size ++;
            *key_store_index = dangling_poor_value;
            std::memcpy (key_store_index + POOR_VALUE_SIZE, key.data(), m_map_conf.key_size);

            sync_ptr (key_store_index, m_hash_element_size);

            return {key_store_index, EMPTY_HIT, dangling_poor_value};
        }

        const auto poor_value = *key_store_index;

        if (dangling_poor_value > poor_value) {
            return {key_store_index, SWAP_HIT, dangling_poor_value};
        }

        key_store_index += m_hash_element_size;
        dangling_poor_value ++;
    }

    return {key_store_index, MATCH_HIT, dangling_poor_value};
}

persisted_robinhood_hashmap::insert_stat persisted_robinhood_hashmap::insert_key(std::span<char> key) {
    auto res = try_place_key(key);
    if (res.hit_stat != SWAP_HIT) {
        return res;
    }

    std::vector <char> swap_space (m_key_value_span_size);
    std::vector <char> dangling_space (m_key_value_span_size);

    std::memcpy (dangling_space.data(), key.data(), m_map_conf.key_size);

    auto dangling_poor_value = res.dangling_poor_value;
    auto key_store_index = res.element_pos;

    insert_stat istat;

    do {
        const auto poor_value = *key_store_index;
        if (dangling_poor_value > poor_value) {
            if (istat.element_pos == nullptr) {
                istat.element_pos = key_store_index;
                istat.dangling_poor_value = dangling_poor_value;
            }
            std::memcpy(swap_space.data(), key_store_index + POOR_VALUE_SIZE, m_key_value_span_size);
            std::memcpy(key_store_index + POOR_VALUE_SIZE, dangling_space.data(), m_key_value_span_size);
            *key_store_index = static_cast <char> (dangling_poor_value);
            dangling_poor_value = poor_value;
            std::swap (swap_space, dangling_space);
        }
        else {
            dangling_poor_value ++;
        }

        key_store_index += m_hash_element_size;
        dangling_poor_value ++;
    } while (std::memcmp (key_store_index + POOR_VALUE_SIZE, m_empty_key.data(), m_map_conf.key_size) != 0);

    m_inserted_keys_size ++;
    std::memcpy (key_store_index + POOR_VALUE_SIZE, dangling_space.data(), m_key_value_span_size);
    *key_store_index = static_cast <char> (dangling_poor_value);

    sync_ptr (key_store_index, m_hash_element_size);

    istat.hit_stat = SWAP_HIT;
    return istat;
}

bool persisted_robinhood_hashmap::need_rehash (const char* inserted_element) const {
    return (static_cast <double> (m_inserted_keys_size) > static_cast <double> (m_key_store.get_size()) * m_map_conf.map_load_factor)
           or (inserted_element >= m_key_store.get_storage() + m_key_store.get_size() - m_hash_element_size)
           or (static_cast <uint8_t> (inserted_element[0]) == std::numeric_limits <std::uint8_t>::max());
}

void persisted_robinhood_hashmap::extend_and_rehash() {

    const auto file_name = m_key_store.get_filename();
    static auto swap_store = [this] (auto& tmp_key_store) {
        std::swap (tmp_key_store, m_key_store);
        auto tmp_atomic_ref = std::atomic_ref <size_t> (*reinterpret_cast <size_t*> (m_key_store.get_storage()));
        std::memcpy(&m_inserted_keys_size, &tmp_atomic_ref, sizeof (tmp_atomic_ref));
    };

    int extension = 2;
    auto tmp_key_store = growing_plain_storage ("tmp", extension * m_key_store.get_size());
    swap_store (tmp_key_store);

    while (!rehash(tmp_key_store)) {
        swap_store (tmp_key_store);
        tmp_key_store.destroy();

        extension *= 2;
        if (extension > m_map_conf.map_maximum_extension_factor) {
            throw std::out_of_range ("persisted_robinhood_hashmap failed to create storage to rehash");
        }

        tmp_key_store = growing_plain_storage ("tmp", extension * m_key_store.get_size());
        swap_store (tmp_key_store);
    }

    tmp_key_store.destroy();
    m_key_store.rename_file(file_name);
}


} // end namespace uh::dbn::storage::smart
