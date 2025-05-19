#include "reference_counter.h"
#include "common/telemetry/log.h"
#include "common/utils/common.h"
#include "common/utils/pointer_traits.h"

namespace uh::cluster {

reference_counter::reference_counter(
    const std::filesystem::path& root, const std::size_t page_size,
    const std::function<std::size_t(std::size_t offset, std::size_t size)>& cb)
    : m_env(lmdb::env::create()),
      m_page_size(page_size),
      m_cb(cb) {
    m_env.set_max_dbs(1);
    m_env.set_mapsize(TEBI_BYTE);
    if (!std::filesystem::exists(root)) {
        std::filesystem::create_directories(root);
    }
    m_env.open(root.c_str(), 0);
}

void reference_counter::execute(std::deque<refcount_cmd>& cmd_queue) {
    std::unordered_map<std::size_t, int> page_operations;

    while (!cmd_queue.empty()) {
        const refcount_cmd& cmd = cmd_queue.front();
        auto page_range = get_page_range(cmd.offset, cmd.size);

        for (std::size_t page_id = page_range.first;
             page_id <= page_range.second; ++page_id) {
            page_operations[page_id] += (cmd.op == INCREMENT) ? 1 : -1;
        }
        cmd_queue.pop_front();
    }

    std::unordered_set<std::size_t> marked_for_deletion;
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    for (const auto& [page_id, net_effect] : page_operations) {
        if (net_effect > 0) {
            increment(page_id, net_effect, false, txn, dbi);
        } else if (net_effect < 0) {
            decrement(page_id, -net_effect, marked_for_deletion, txn, dbi);
        }
    }
    txn.commit();

    free_storage(marked_for_deletion);
}
std::pair<std::size_t, std::size_t>
reference_counter::get_page_range(std::size_t offset, std::size_t size) const {
    return std::make_pair(offset / m_page_size,
                          (offset + size - 1) / m_page_size);
}

address reference_counter::increment(const address& addr) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    address rv;

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        auto page_range = get_page_range(frag.pointer, frag.size);
        for (size_t page_id = page_range.first; page_id <= page_range.second;
             page_id++) {
            if (increment(page_id, 1, true, txn, dbi)) {
                rv.push(frag);
                break;
            }
        }
    }

    txn.commit();
    return rv;
}

size_t reference_counter::decrement(const address& addr) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    std::unordered_set<std::size_t> pages_to_free;

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        auto page_range = get_page_range(frag.pointer, frag.size);
        for (size_t page_id = page_range.first; page_id <= page_range.second;
             page_id++) {
            decrement(page_id, 1, pages_to_free, txn, dbi);
        }
    }
    txn.commit();

    return free_storage(pages_to_free);
}

bool reference_counter::increment(const std::size_t page_id,
                                  const std::size_t count, bool upstream,
                                  lmdb::txn& txn, lmdb::dbi& dbi) {
    auto key = lmdb::to_sv<std::size_t>(page_id);
    std::string_view view;

    std::size_t current_value = 0;
    bool encountered_untracked_page = false;

    if (dbi.get(txn, key, view)) {
        current_value = lmdb::from_sv<std::size_t>(view);
    } else {
        encountered_untracked_page = true;
    }

    if (upstream && encountered_untracked_page) {
        LOG_DEBUG() << "encountered untracked page: " << page_id;
        return true;
    } else {
        dbi.put(txn, key, lmdb::to_sv<std::size_t>(current_value + count));
        return false;
    }
}

void reference_counter::decrement(
    const std::size_t page_id, const std::size_t count,
    std::unordered_set<std::size_t>& marked_for_deletion, lmdb::txn& txn,
    lmdb::dbi& dbi) {

    auto key = lmdb::to_sv<std::size_t>(page_id);
    std::string_view view;

    if (!dbi.get(txn, key, view)) {
        txn.abort();
        std::string msg =
            "decreasing refcount of un-tracked page " + std::to_string(page_id);
        LOG_ERROR() << msg;
        throw std::runtime_error(msg);
    }

    std::size_t current_value = lmdb::from_sv<std::size_t>(view);
    if (current_value <= count) {
        marked_for_deletion.emplace(page_id);
        dbi.del(txn, key);
    } else {
        dbi.put(txn, key, lmdb::to_sv<std::size_t>(current_value - count));
    }
}

size_t reference_counter::free_storage(
    std::unordered_set<std::size_t>& pages_to_free) {
    if (pages_to_free.empty()) {
        return 0;
    }

    std::vector<std::size_t> sorted_pages(pages_to_free.begin(),
                                          pages_to_free.end());
    std::sort(sorted_pages.begin(), sorted_pages.end());

    size_t freed_storage = 0;

    std::size_t range_start = sorted_pages.front();
    std::size_t range_end = range_start;

    for (size_t i = 1; i < sorted_pages.size(); ++i) {
        if (sorted_pages[i] == range_end + 1) {
            range_end = sorted_pages[i];
        } else {
            std::size_t del_offset = range_start * m_page_size;
            std::size_t del_size = (range_end - range_start + 1) * m_page_size;
            freed_storage += this->m_cb(del_offset, del_size);

            range_start = range_end = sorted_pages[i];
        }
    }

    std::size_t del_offset = range_start * m_page_size;
    std::size_t del_size = (range_end - range_start + 1) * m_page_size;
    freed_storage += this->m_cb(del_offset, del_size);

    return freed_storage;
}

} // namespace uh::cluster
