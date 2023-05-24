//
// Created by masi on 5/24/23.
//
#include "robin_hood_hashmap.h"
#include "smart_storage.h"

namespace uh::dbn::storage::smart {
robin_hood_hashmap::robin_hood_hashmap(std::filesystem::path key_file, mmap_storage &value_store) :
        m_key_file (std::move (key_file)),
        m_key_store (init_mmap(m_key_file, m_key_file_size, m_key_file_size)),
        m_value_store (value_store),
        m_inserted_keys_size {*reinterpret_cast <size_t*> (m_key_store)} {

    std::memset (m_empty_spot, 0, KEY_SIZE);
}

void robin_hood_hashmap::insert(std::span<char> key, std::span<char> value) {

    auto index = hash_index (key);
    const std::uint32_t value_size = value.size_bytes();
    auto key_store_index = m_key_store + index + POOR_VALUE_SIZE;
    std::uint8_t dangling_poor_value = 0;
    boost::upgrade_lock <boost::shared_mutex> lock (m_mutex);

    bool swap_element = false;

    while (std::memcmp (key_store_index, key.data(), KEY_SIZE) != 0) {

        if (std::memcmp (key_store_index, m_empty_spot, KEY_SIZE) == 0) {


            const auto offset_ptr = m_value_store.allocate(value.size_bytes());
            const auto value_ptr = m_value_store.get_raw_ptr(offset_ptr.m_offset);
            std::memcpy (value_ptr, value.data(), value_size);

            boost::upgrade_to_unique_lock write_lock (lock);

            std::memcpy (key_store_index, key.data(), KEY_SIZE);
            std::memcpy (key_store_index + KEY_SIZE, &offset_ptr.m_offset, VALUE_PTR_SIZE);
            std::memcpy (key_store_index + KEY_SIZE + VALUE_PTR_SIZE, &value_size, VALUE_LENGTH_SIZE);

            m_key_store [index] = dangling_poor_value;
            increment_inserted_key_size ();

            if (msync (align_ptr (key_store_index), HASH_ELEMENT_SIZE, MS_SYNC) != 0) {
                throw std::system_error (errno, std::system_category(), "robin_hood_hashmap could not sync the mmap data");
            }
            m_value_store.sync(value_ptr, value_size);


            break;
        }

        const auto poor_value = m_key_store [index];

        if (dangling_poor_value > poor_value) {
            swap_element = true;
            break;
        }

        key_store_index += HASH_ELEMENT_SIZE;
        index += HASH_ELEMENT_SIZE;
        dangling_poor_value ++;

    }

    if (!swap_element) {
        return;
    }

    char swap_space [KEY_VALUE_SPAN_SIZE];
    char dangling_space [KEY_VALUE_SPAN_SIZE];

    const auto offset_ptr = m_value_store.allocate(value.size_bytes());
    const auto value_ptr = m_value_store.get_raw_ptr(offset_ptr.m_offset);
    std::memcpy (value_ptr, value.data(), value_size);

    boost::upgrade_to_unique_lock write_lock (lock);

    std::memcpy (dangling_space, key.data(), KEY_SIZE);
    std::memcpy (dangling_space + KEY_SIZE, &offset_ptr.m_offset, VALUE_PTR_SIZE);
    std::memcpy (dangling_space + KEY_SIZE + VALUE_PTR_SIZE, &value_size, VALUE_LENGTH_SIZE);

    const auto modification_start = index;

    do {
        const auto poor_value = m_key_store [index];
        if (dangling_poor_value > poor_value) {
            std::memcpy(swap_space, key_store_index, KEY_VALUE_SPAN_SIZE);
            m_key_store [index] = dangling_poor_value;
            std::memcpy(key_store_index, dangling_space, KEY_VALUE_SPAN_SIZE);
            dangling_poor_value = poor_value;
            std::swap (swap_space, dangling_space);
        }
        else {
            dangling_poor_value ++;
        }

        key_store_index += HASH_ELEMENT_SIZE;
        index += HASH_ELEMENT_SIZE;
        dangling_poor_value ++;
    } while (std::memcmp (key_store_index, m_empty_spot, KEY_SIZE) != 0);

    m_key_store [index] = dangling_poor_value;
    std::memcpy (key_store_index, dangling_space, KEY_VALUE_SPAN_SIZE);

    increment_inserted_key_size ();

    lock.unlock();

    if (msync (align_ptr (m_key_store + modification_start), index + HASH_ELEMENT_SIZE, MS_SYNC) != 0) {
        throw std::system_error (errno, std::system_category(), "robin_hood_hashmap could not sync the mmap data");
    }

    m_value_store.sync(value_ptr, value_size);

}

std::optional<std::span<char>> robin_hood_hashmap::get(std::span<char> key) {
    auto index = hash_index (key);

    std::optional <std::span <char>> res;
    boost::shared_lock <boost::shared_mutex> lock (m_mutex);

    uint8_t expected_poor_value = 0;
    while (std::memcmp (m_key_store + index + POOR_VALUE_SIZE, key.data(), KEY_SIZE) != 0) {

        if (expected_poor_value > m_key_store [index]) {
            return res;
        }

        expected_poor_value ++;
        index += HASH_ELEMENT_SIZE;
    }

    const auto value_offset = *reinterpret_cast <uint64_t*> (m_key_store + index + POOR_VALUE_SIZE + KEY_SIZE);
    const auto value_size = *reinterpret_cast <uint32_t *> (m_key_store + index + POOR_VALUE_SIZE + KEY_SIZE + VALUE_PTR_SIZE);

    const auto value_ptr = m_value_store.get_raw_ptr(value_offset);
    res.emplace(static_cast <char *> (value_ptr), value_size);
    return res;
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
    return h % m_key_file_size + KEY_STORE_META_DATA_SIZE;
}

void robin_hood_hashmap::increment_inserted_key_size() {
    m_inserted_keys_size ++;
    if (static_cast <double> (m_inserted_keys_size) > static_cast <double> (m_key_file_size) * KEY_STORE_LOAD_FACTOR) {
        msync (m_key_store, m_key_file_size, MS_SYNC);
        munmap (m_key_store, m_key_file_size);

        m_key_file_size *= 2;

        const auto fd = open(m_key_file.c_str(), O_RDWR);
        ftruncate(fd, m_key_file_size);

        m_key_store = static_cast <char*> (mmap(nullptr, m_key_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        close(fd);
    }
}
} // end namespace uh::dbn::storage::smart
