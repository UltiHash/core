#include "reference_counter.h"
#include "common/utils/common.h"
#include "common/utils/pointer_traits.h"

namespace uh::cluster {

reference_counter::reference_counter(
    const std::filesystem::path& root, const std::size_t page_size,
    const std::function<void(std::size_t offset, std::size_t size)>& cb)
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

void reference_counter::decrement(std::size_t offset, std::size_t size) {
    address addr;
    addr.push({offset, size});
    decrement(addr);
}

void reference_counter::decrement(const address& addr) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        const auto offset = pointer_traits::get_pointer(frag.pointer);

        std::vector<std::pair<std::size_t, std::size_t>> marked_for_deletion;
        std::optional<std::size_t> deleteRangeStart;
        std::optional<std::size_t> deleteRangeEnd;

        for (std::size_t page_pointer = offset;
             page_pointer < offset + frag.size; page_pointer += m_page_size) {
            std::size_t page_id = page_pointer / m_page_size;
            std::string key(std::to_string(page_id));
            std::string_view value;

            if (!dbi.get(txn, key, value)) {
                txn.abort();
                throw std::runtime_error(
                    "attempted to to decrease refcount of an "
                    "un-tracked page at page " +
                    std::to_string(page_id));
            }

            std::size_t current_value = std::stoull(std::string(value));

            if (current_value == 0) {
                txn.abort();
                throw std::runtime_error("encountered page with refcount zero, "
                                         "even though such entries "
                                         "should not exist");
            }

            if (--current_value == 0) {
                if (deleteRangeStart.has_value() &&
                    deleteRangeEnd.has_value() &&
                    page_id == deleteRangeEnd.value() + 1) {
                    deleteRangeEnd = page_id;
                } else {
                    if (deleteRangeStart.has_value() &&
                        deleteRangeEnd.has_value()) {
                        marked_for_deletion.emplace_back(
                            deleteRangeStart.value(), deleteRangeEnd.value());
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

        for (auto range : marked_for_deletion) {
            std::size_t del_offset = range.first * m_page_size;
            std::size_t del_size =
                (range.second - range.first) * m_page_size + m_page_size;
            m_cb(del_offset, del_size);
        }
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
        std::string key(std::to_string(page_id));
        std::string_view value;

        std::size_t current_value = 0;
        if (dbi.get(txn, key, value)) {
            current_value = std::stoull(std::string(value));
        }

        std::string value_str(std::to_string(++current_value));
        dbi.put(txn, key, value_str);
    }

    txn.commit();
}

address reference_counter::increment(const address& addr) {
    lmdb::txn txn = lmdb::txn::begin(m_env, nullptr, 0);
    lmdb::dbi dbi = lmdb::dbi::open(txn, nullptr);
    address rv;

    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        const auto offset = pointer_traits::get_pointer(frag.pointer);
        for (std::size_t page_pointer = offset;
             page_pointer < offset + frag.size; page_pointer += m_page_size) {
            std::size_t page_id = page_pointer / m_page_size;
            std::string key(std::to_string(page_id));
            std::string_view value;

            if (dbi.get(txn, key, value)) {
                std::size_t current_value = std::stoull(std::string(value));
                std::string value_str(std::to_string(++current_value));
                dbi.put(txn, key, value_str);
            } else {
                rv.push(frag);
            }
        }
    }

    txn.commit();
    return rv;
}

} // namespace uh::cluster