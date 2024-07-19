#include "reference_counter.h"
#include "common/utils/common.h"

namespace uh::cluster {

reference_counter::reference_counter(const std::filesystem::path& root,
                                     const std::size_t page_size)
    : m_env(lmdb::env::create()),
      m_page_size(page_size) {
    m_env.set_max_dbs(1);
    m_env.set_mapsize(TEBI_BYTE);
    if (!std::filesystem::exists(root)) {
        std::filesystem::create_directories(root);
    }
    m_env.open(root.c_str(), 0);
}

void reference_counter::decrement(const std::size_t offset,
                                  const std::size_t size) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    for (std::size_t page_pointer = offset; page_pointer < offset + size;
         page_pointer += m_page_size) {
        std::size_t page_id = page_pointer / m_page_size;
        lmdb::val key(&page_id, sizeof(page_id));
        lmdb::val value;

        if (!dbi.get(txn, key, value)) {
            txn.abort();
            throw std::runtime_error("key does not exist");
        }

        std::size_t current_value;
        std::memcpy(&current_value, value.data(), sizeof(current_value));

        if (current_value == 0) {
            txn.abort();
            throw std::runtime_error("reference counter is already at zero");
        }

        --current_value;
        value = lmdb::val(&current_value, sizeof(current_value));
        dbi.put(txn, key, value);
    }

    txn.commit();
}

void reference_counter::increment(const std::size_t offset,
                                  const std::size_t size) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    for (std::size_t page_pointer = offset; page_pointer < offset + size;
         page_pointer += m_page_size) {
        std::size_t page_id = page_pointer / m_page_size;
        lmdb::val key(&page_id, sizeof(page_id));
        lmdb::val value;

        std::size_t current_value = 0;
        if (dbi.get(txn, key, value)) {
            std::memcpy(&current_value, value.data(), sizeof(current_value));
        }

        ++current_value;
        value = lmdb::val(&current_value, sizeof(current_value));
        dbi.put(txn, key, value);
    }

    txn.commit();
}

} // namespace uh::cluster