//
// Created by masi on 5/24/23.
//
#include "robin_hood_hashmap.h"
#include "smart_storage.h"

namespace uh::dbn::storage::smart {
robin_hood_hashmap::robin_hood_hashmap(size_t key_size, std::filesystem::path key_file, std::filesystem::path value_directory) :
        m_key_size (key_size),
        m_key_value_span_size (m_key_size + VALUE_PTR_SIZE + VALUE_LENGTH_SIZE),
        m_hash_element_size (POOR_VALUE_SIZE + m_key_value_span_size),
        m_empty_key (m_key_size),
        m_key_file (std::move (key_file)),
        m_key_store (init_mmap(m_key_file, m_key_file_size, m_key_file_size)),
        m_value_store (std::move (value_directory), MIN_VALUE_FILE_SIZE, MAX_VALUE_FILE_SIZE),
        m_inserted_keys_size {*reinterpret_cast <size_t*> (m_key_store)} {
    std::memset (m_empty_key.data(), 0, m_key_size);
}

void robin_hood_hashmap::insert(std::span<char> key, std::span<char> value) {

    std::unique_lock <std::shared_mutex> lock (m_mutex);

    auto res = insert_key (key);
    if (std::get <0> (res) == NON_HIT) {
        extend_key_store();
        res = insert_key (key);
        if (std::get <0> (res) == NON_HIT) {
            throw std::bad_alloc ();
        }
    }

    const std::uint32_t value_size = value.size_bytes();
    const auto offset_ptr = m_value_store.allocate(value.size_bytes());

    const auto key_store_value_index = std::get <2> (res) + POOR_VALUE_SIZE + m_key_size;
    std::memcpy (key_store_value_index, &offset_ptr.m_offset, VALUE_PTR_SIZE);
    std::memcpy (key_store_value_index + VALUE_PTR_SIZE, &value_size, VALUE_LENGTH_SIZE);

    if (static_cast <double> (m_inserted_keys_size) > static_cast <double> (m_key_file_size) * KEY_STORE_LOAD_FACTOR) {
        extend_key_store ();
    }

    lock.unlock();

    const auto value_ptr = m_value_store.get_raw_ptr(offset_ptr.m_offset);
    std::memcpy (value_ptr, value.data(), value_size);

}

std::optional<std::span<char>> robin_hood_hashmap::get(std::span<char> key) {
    auto index = hash_index (key);

    std::shared_lock <std::shared_mutex> lock (m_mutex);

    uint8_t expected_poor_value = 0;
    while (std::memcmp (m_key_store + index + POOR_VALUE_SIZE, key.data(), m_key_size) != 0) {

        if (expected_poor_value > m_key_store [index]) {
            return std::nullopt;
        }

        expected_poor_value ++;
        index += m_hash_element_size;
    }

    const auto value_offset = *reinterpret_cast <uint64_t*> (m_key_store + index + POOR_VALUE_SIZE + m_key_size);
    const auto value_size = *reinterpret_cast <uint32_t*> (m_key_store + index + POOR_VALUE_SIZE + m_key_size + VALUE_PTR_SIZE);

    const auto value_ptr = m_value_store.get_raw_ptr(value_offset);
    return {std::span <char> (static_cast <char *> (value_ptr), value_size)};
}

robin_hood_hashmap::~robin_hood_hashmap() {
    msync (m_key_store, m_key_file_size, MS_SYNC);
    m_value_store.sync();
    munmap(m_key_store, m_key_file_size);
}

size_t robin_hood_hashmap::hash_index(const std::span<char> &key) const noexcept {
    size_t h = 0ul;
    for (int i = 0; i < 2 * sizeof (size_t); i += sizeof (size_t)) {
        h += *reinterpret_cast <size_t*> (key.data() + i);
    }
    auto pos = h % m_key_file_size;
    pos += m_hash_element_size - pos % m_hash_element_size;

    return pos + KEY_STORE_META_DATA_SIZE;
}

void robin_hood_hashmap::extend_key_store () {
    msync (m_key_store, m_key_file_size, MS_SYNC);
    munmap (m_key_store, m_key_file_size);

    m_key_file_size *= 2;

    const auto fd = open(m_key_file.c_str(), O_RDWR);
    ftruncate(fd, static_cast <long> (m_key_file_size));

    m_key_store = static_cast <char*> (mmap(nullptr, m_key_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    rehash (m_key_file_size / 2, m_key_file_size);
}

void robin_hood_hashmap::rehash(size_t old_file_size, size_t new_file_size) {
    const auto end_old_key_store = reinterpret_cast <char*> (m_key_store + old_file_size);
    std::vector <char> tmp (m_key_value_span_size);
    for (char* i = m_key_store + KEY_STORE_META_DATA_SIZE + POOR_VALUE_SIZE; i < end_old_key_store; i += m_hash_element_size) {
        if (std::memcmp (i, m_empty_key.data(), m_key_size) != 0) {
            std::memcpy (tmp.data(), i, m_key_value_span_size);
            std::memset (i - POOR_VALUE_SIZE, 0, m_key_size + POOR_VALUE_SIZE);
            const auto res = insert_key({tmp.data(), m_key_size});
            if (std::get <0> (res) == NON_HIT) {
                extend_key_store();
                return;
            }
            std::memcpy (std::get <2> (res) + POOR_VALUE_SIZE + m_key_size, tmp.data() + m_key_size, VALUE_PTR_SIZE + VALUE_LENGTH_SIZE);
        }
    }
}

std::tuple <robin_hood_hashmap::hit_stat, uint8_t, char*> robin_hood_hashmap::try_place_key(std::span<char> key) {

    auto key_store_index = m_key_store + hash_index (key);
    std::uint8_t dangling_poor_value = 0;

    while (std::memcmp (key_store_index + POOR_VALUE_SIZE, key.data(), m_key_size) != 0) {

        if (std::memcmp (key_store_index + POOR_VALUE_SIZE, m_empty_key.data(), m_key_size) == 0) {

            if (key_store_index + m_hash_element_size > m_key_store + m_key_file_size) {
                return {NON_HIT, dangling_poor_value, nullptr};
            }

            m_inserted_keys_size ++;
            *key_store_index = dangling_poor_value;
            std::memcpy (key_store_index + POOR_VALUE_SIZE, key.data(), m_key_size);

            if (msync (align_ptr (key_store_index), m_hash_element_size, MS_SYNC) != 0) {
                throw std::system_error (errno, std::system_category(), "robin_hood_hashmap could not sync the mmap data");
            }

            return {EMPTY_HIT, dangling_poor_value, key_store_index};
        }

        const auto poor_value = *key_store_index;

        if (dangling_poor_value > poor_value) {
            return {SWAP_HIT, dangling_poor_value, key_store_index};
        }

        key_store_index += m_hash_element_size;
        dangling_poor_value ++;
    }

    return {MATCH_HIT, dangling_poor_value, key_store_index};
}

std::tuple <robin_hood_hashmap::hit_stat, uint8_t, char*> robin_hood_hashmap::insert_key(std::span<char> key) {
    const auto res = try_place_key(key);
    if (std::get <0> (res) != SWAP_HIT) {
        return res;
    }

    std::vector <char> swap_space (m_key_value_span_size);
    std::vector <char> dangling_space (m_key_value_span_size);

    std::memcpy (dangling_space.data(), key.data(), m_key_size);

    auto dangling_poor_value = std::get <1> (res);
    auto key_store_index = std::get <2> (res);
    const auto modification_start = key_store_index;

    do {
        const auto poor_value = *key_store_index;
        if (dangling_poor_value > poor_value) {
            std::memcpy(swap_space.data(), key_store_index + POOR_VALUE_SIZE, m_key_value_span_size);
            *key_store_index = static_cast <char> (dangling_poor_value);
            std::memcpy(key_store_index + POOR_VALUE_SIZE, dangling_space.data(), m_key_value_span_size);
            dangling_poor_value = poor_value;
            std::swap (swap_space, dangling_space);
        }
        else {
            dangling_poor_value ++;
        }

        key_store_index += m_hash_element_size;
        dangling_poor_value ++;
    } while (std::memcmp (key_store_index + POOR_VALUE_SIZE, m_empty_key.data(), m_key_size) != 0);

    if (key_store_index + m_hash_element_size > m_key_store + m_key_file_size) {
        return {NON_HIT, dangling_poor_value, nullptr};
    }

    m_inserted_keys_size ++;
    *key_store_index = static_cast <char> (dangling_poor_value);
    std::memcpy (key_store_index + POOR_VALUE_SIZE, dangling_space.data(), m_key_value_span_size);

    const auto aligned = static_cast <char*> (align_ptr (modification_start));
    if (msync (aligned, key_store_index + m_hash_element_size - aligned, MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "robin_hood_hashmap could not sync the mmap data");
    }

    return {SWAP_HIT, dangling_poor_value, key_store_index};
}


} // end namespace uh::dbn::storage::smart
