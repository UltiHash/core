#include "reference_counter.h"
#include "common/utils/common.h"

namespace uh::cluster {

reference_counter::reference_counter(const std::filesystem::path& root) : m_env(lmdb::env::create()) {
    m_env.set_max_dbs(1);
    m_env.set_mapsize(TEBI_BYTE);
    if (!std::filesystem::exists(root)) {
        std::filesystem::create_directories(root);
    }
    m_env.open(root.c_str(), 0);
}

void reference_counter::initialize(const std::set<std::size_t>& pages) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    std::size_t init_value = 1;

    for(const std::size_t& page_id : pages) {
        lmdb::val key(&page_id, sizeof(page_id));
        lmdb::val value(&init_value, sizeof(init_value));
        dbi.put(txn, key, value);
    }

    txn.commit();
}

std::map<std::size_t, std::size_t> reference_counter::decrement(const std::set<std::size_t>& pages) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    std::map<std::size_t, std::size_t> refcount_by_pageid;

    for(const std::size_t& page_id : pages) {
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
        refcount_by_pageid.insert({page_id, current_value});
    }

    txn.commit();
    return refcount_by_pageid;
}

std::map<std::size_t, std::size_t> reference_counter::increment(const std::set<std::size_t>& pages) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    std::map<std::size_t, std::size_t> refcount_by_pageid;

    for(const std::size_t& page_id : pages) {
        lmdb::val key(&page_id, sizeof(page_id));
        lmdb::val value;

        if (!dbi.get(txn, key, value)) {
            txn.abort();
            throw std::runtime_error("key does not exist");
        }

        std::size_t current_value = 0;
        if (dbi.get(txn, key, value)) {
            std::memcpy(&current_value, value.data(), sizeof(current_value));
        }

        ++current_value;
        value = lmdb::val(&current_value, sizeof(current_value));
        dbi.put(txn, key, value);
        refcount_by_pageid.insert({page_id, current_value});
    }


    txn.commit();
    return refcount_by_pageid;
}

/*
std::size_t reference_counter::at(std::size_t page_id) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, MDB_RDONLY);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    lmdb::val key(&page_id, sizeof(page_id));
    lmdb::val value;

    std::size_t current_value = 0;
    if (dbi.get(txn, key, value)) {
        std::memcpy(&current_value, value.data(), sizeof(current_value));
    }

    txn.abort();
    return current_value;
}
*/

}