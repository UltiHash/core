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

size_t reference_counter::decrement(const address& addr) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    std::vector<std::pair<std::size_t, std::size_t>> marked_for_deletion;

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        const auto offset = pointer_traits::get_pointer(frag.pointer);
        const auto size = frag.size;

        decrement(offset, size, marked_for_deletion, txn, dbi);
    }
    txn.commit();

    return call_cb(marked_for_deletion);
}

void reference_counter::decrement(
    const std::size_t offset, const std::size_t size,
    std::vector<std::pair<std::size_t, std::size_t>>& marked_for_deletion,
    lmdb::txn& txn, lmdb::dbi& dbi) {

    std::optional<std::size_t> deleteRangeStart;
    std::optional<std::size_t> deleteRangeEnd;

    std::size_t start_page = offset / this->m_page_size;
    std::size_t end_page = (offset + size - 1) / this->m_page_size;

    LOG_DEBUG() << "decrementing refcount at offset=" << offset
                << ", size=" << size;
    for (std::size_t page_id = start_page; page_id <= end_page; ++page_id) {
        std::string key(std::to_string(page_id));
        std::string_view value;

        if (!dbi.get(txn, key, value)) {
            txn.abort();
            std::string msg = "decreasing refcount of un-tracked page " +
                              std::to_string(page_id);
            LOG_ERROR() << msg;
            throw std::runtime_error(msg);
        }

        std::size_t current_value = std::stoull(std::string(value));

        if (current_value == 0) {
            txn.abort();
            std::string msg = "encountered page with refcount zero";
            LOG_ERROR() << msg;
            throw std::runtime_error(msg);
        }

        if (--current_value == 0) {
            if (deleteRangeStart.has_value() && deleteRangeEnd.has_value() &&
                page_id == deleteRangeEnd.value() + 1) {
                deleteRangeEnd = page_id;
            } else {
                if (deleteRangeStart.has_value() &&
                    deleteRangeEnd.has_value()) {
                    marked_for_deletion.emplace_back(deleteRangeStart.value(),
                                                     deleteRangeEnd.value());
                }
                deleteRangeStart = page_id;
                deleteRangeEnd = page_id;
            }

            dbi.del(txn, key);
        } else {
            std::string value_str = std::to_string(current_value);
            dbi.put(txn, key, value_str);
        }
    }

    if (deleteRangeStart.has_value() && deleteRangeEnd.has_value()) {
        marked_for_deletion.emplace_back(deleteRangeStart.value(),
                                         deleteRangeEnd.value());
    }
}

size_t reference_counter::call_cb(
    std::vector<std::pair<std::size_t, std::size_t>>& marked_for_deletion) {
    size_t freed_space = 0;
    for (auto range : marked_for_deletion) {
        std::size_t del_offset = range.first * this->m_page_size;
        std::size_t del_size =
            (range.second - range.first) * this->m_page_size +
            this->m_page_size;
        freed_space += this->m_cb(del_offset, del_size);
    }
    return freed_space;
}

void reference_counter::increment(const std::size_t offset,
                                  const std::size_t size) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    increment(offset, size, txn, dbi);

    txn.commit();
}

void reference_counter::increment(const std::size_t offset,
                                  const std::size_t size, lmdb::txn& txn,
                                  lmdb::dbi& dbi) {
    std::size_t start_page = offset / m_page_size;
    std::size_t end_page = (offset + size - 1) / m_page_size;

    LOG_DEBUG() << "incrementing refcount at offset=" << offset
                << ", size=" << size;
    for (std::size_t page_id = start_page; page_id <= end_page; ++page_id) {
        std::string key(std::to_string(page_id));
        std::string_view value;

        std::size_t current_value = 0;
        if (dbi.get(txn, key, value)) {
            current_value = std::stoull(std::string(value));
        }

        std::string value_str(std::to_string(++current_value));
        dbi.put(txn, key, value_str);
    }
}

address reference_counter::increment(const address& addr) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    address rv;

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        const auto offset = pointer_traits::get_pointer(frag.pointer);
        std::map<std::size_t, std::size_t> value_by_page;
        bool encountered_untracked_page = false;

        std::size_t start_page = offset / m_page_size;
        std::size_t end_page = (offset + frag.size - 1) / m_page_size;

        LOG_DEBUG() << "incrementing refcount at offset=" << offset
                    << ", size=" << frag.size << " (on link)";
        for (std::size_t page_id = start_page; page_id <= end_page; ++page_id) {
            std::string key(std::to_string(page_id));
            std::string_view value;

            if (dbi.get(txn, key, value)) {
                std::size_t current_value = std::stoull(std::string(value));
                value_by_page.emplace(page_id, ++current_value);
            } else {
                encountered_untracked_page = true;
                break;
            }
        }

        if (encountered_untracked_page) {
            rv.push(frag);
        } else {
            for (auto& pair : value_by_page) {
                std::string key(std::to_string(pair.first));
                std::string val(std::to_string(pair.second));
                dbi.put(txn, key, val);
            }
        }
    }

    txn.commit();
    return rv;
}

void reference_counter::enqueue_increment(std::size_t offset,
                                          std::size_t size) {
    m_cmd_queue.emplace_back(INCREMENT, offset, size);
}
void reference_counter::enquque_decrement(std::size_t offset,
                                          std::size_t size) {
    m_cmd_queue.emplace_back(DECREMENT, offset, size);
}

std::size_t reference_counter::flush_queue() {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    std::vector<std::pair<std::size_t, std::size_t>> marked_for_deletion;

    for (; !m_cmd_queue.empty(); m_cmd_queue.pop_front()) {
        auto& cmd = m_cmd_queue.front();
        switch (cmd.op) {
        case INCREMENT:
            increment(cmd.offset, cmd.size, txn, dbi);
            break;
        case DECREMENT:
            decrement(cmd.offset, cmd.size, marked_for_deletion, txn, dbi);
            break;
        default:
            txn.abort();
            std::string msg = "encountered invalid operation in "
                              "reference_counter::flush_queue()";
            LOG_ERROR() << msg;
            throw std::runtime_error(msg);
        }
    }

    txn.commit();
    return call_cb(marked_for_deletion);
}

} // namespace uh::cluster
