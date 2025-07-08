#include "reference_counter.h"
#include "common/telemetry/log.h"
#include "common/utils/common.h"
#include "common/utils/pointer_traits.h"

namespace uh::cluster {

reference_counter::reference_counter(
    const std::filesystem::path& root, const std::size_t stripe_size,
    const std::function<std::size_t(std::size_t offset, std::size_t size)>& cb)
    : m_env(lmdb::env::create()),
      m_stripe_size(stripe_size),
      m_cb(cb) {
    m_env.set_max_dbs(1);
    m_env.set_mapsize(TEBI_BYTE);
    if (!std::filesystem::exists(root)) {
        std::filesystem::create_directories(root);
    }
    m_env.open(root.c_str(), 0);
}

std::vector<refcount_t>
reference_counter::increment(const std::vector<refcount_t>& refcounts,
                             bool upstream) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    std::vector<refcount_t> rv;

    for (const auto& refcount : refcounts) {
        if (increment(refcount.stripe_id, refcount.count, upstream, txn, dbi)) {
            rv.emplace_back(refcount);
        }
    }

    txn.commit();
    return rv;
}

std::size_t
reference_counter::decrement(const std::vector<refcount_t>& refcounts) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    std::vector<std::size_t> stripes_to_free;

    for (auto& refcount : refcounts) {
        if (decrement(refcount.stripe_id, refcount.count, txn, dbi)) {
            stripes_to_free.push_back(refcount.stripe_id);
        }
    }
    txn.commit();

    return free_stripes(stripes_to_free);
}

std::vector<refcount_t>
reference_counter::get_refcounts(const std::vector<std::size_t>& stripe_ids) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    std::vector<refcount_t> rv;

    for (auto stripe_id : stripe_ids) {
        auto key = lmdb::to_sv<std::size_t>(stripe_id);
        std::string_view view;
        std::size_t current_value = 0;

        if (dbi.get(txn, key, view)) {
            current_value = lmdb::from_sv<std::size_t>(view);
        }

        rv.emplace_back(stripe_id, current_value);
    }
    txn.abort();
    return rv;
}

bool reference_counter::increment(const std::size_t stripe_id,
                                  const std::size_t count, bool upstream,
                                  lmdb::txn& txn, lmdb::dbi& dbi) {
    auto key = lmdb::to_sv<std::size_t>(stripe_id);
    std::string_view view;

    std::size_t current_value = 0;
    bool encountered_untracked_page = false;

    if (dbi.get(txn, key, view)) {
        current_value = lmdb::from_sv<std::size_t>(view);
    } else {
        encountered_untracked_page = true;
    }

    if (upstream && encountered_untracked_page) {
        LOG_DEBUG() << "encountered untracked page: " << stripe_id;
        return true;
    } else {
        dbi.put(txn, key, lmdb::to_sv<std::size_t>(current_value + count));
        return false;
    }
}

bool reference_counter::decrement(const std::size_t stripe_id,
                                  const std::size_t count, lmdb::txn& txn,
                                  lmdb::dbi& dbi) {

    auto key = lmdb::to_sv<std::size_t>(stripe_id);
    std::string_view view;

    if (!dbi.get(txn, key, view)) {
        txn.abort();
        std::string msg = "decreasing refcount of un-tracked page " +
                          std::to_string(stripe_id);
        LOG_ERROR() << msg;
        throw std::runtime_error(msg);
    }

    std::size_t current_value = lmdb::from_sv<std::size_t>(view);
    if (current_value <= count) {
        dbi.del(txn, key);
        return true;
    } else {
        dbi.put(txn, key, lmdb::to_sv<std::size_t>(current_value - count));
        return false;
    }
}

std::size_t
reference_counter::free_stripes(std::vector<std::size_t>& stripes_to_free) {
    if (stripes_to_free.empty()) {
        return 0;
    }
    size_t freed_storage = 0;

    std::size_t range_start = stripes_to_free.front();
    std::size_t range_end = range_start;

    for (size_t i = 1; i < stripes_to_free.size(); ++i) {
        if (stripes_to_free[i] == range_end + 1) {
            range_end = stripes_to_free[i];
        } else {
            std::size_t del_offset = range_start * m_stripe_size;
            std::size_t del_size =
                (range_end - range_start + 1) * m_stripe_size;
            freed_storage += this->m_cb(del_offset, del_size);

            range_start = range_end = stripes_to_free[i];
        }
    }

    std::size_t del_offset = range_start * m_stripe_size;
    std::size_t del_size = (range_end - range_start + 1) * m_stripe_size;
    freed_storage += this->m_cb(del_offset, del_size);

    return freed_storage;
}

} // namespace uh::cluster
