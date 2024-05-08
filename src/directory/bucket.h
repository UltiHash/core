#ifndef CORE_BUCKET_H
#define CORE_BUCKET_H

#include "chaining_data_store.h"
#include "common/utils/error.h"
#include "common/utils/time_utils.h"
#include "config.h"
#include "transaction_log.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace uh::cluster {

struct object_meta {
    address addr;
    utc_time last_modified;

    constexpr static auto serialize(auto& archive, auto& self) {
        std::size_t count = 0;
        auto res = archive(self.addr, count);

        self.last_modified = utc_time(utc_time::duration(count));
        return res;
    }

    constexpr static auto serialize(auto& archive, const auto& self) {
        std::size_t count = self.last_modified.time_since_epoch().count();
        return archive(self.addr, count);
    }
};

class bucket {

public:
    bucket(const std::filesystem::path& root, const std::string& bucket_name,
           const bucket_config& conf)
        : m_bucket_path(root / bucket_name),
          m_data_store({
              .directory = m_bucket_path / "ds",
              .free_spot_log = m_bucket_path / "ds/free_spot_log",
              .min_file_size = conf.min_file_size,
              .max_file_size = conf.max_file_size,
              .max_storage_size = conf.max_storage_size,
              .max_chunk_size = conf.max_chunk_size,
          }),
          m_transaction_log(m_bucket_path / "transaction_log"),
          m_object_ptrs(m_transaction_log.replay()) {}

    std::vector<object> list_objects(const std::string& lower_bound,
                                     const std::string& prefix) {
        std::vector<object> objects;

        auto start = m_object_ptrs.cbegin();
        if (!prefix.empty()) {
            start = m_object_ptrs.lower_bound(prefix);
        } else if (!lower_bound.empty() && lower_bound > prefix) {
            start = m_object_ptrs.upper_bound(lower_bound);
        }

        for (; start != m_object_ptrs.end(); ++start) {
            if (!lower_bound.empty() && start->first.starts_with(lower_bound)) {
                continue;
            } else if (start->first.starts_with(prefix)) {
                const auto bytes = m_data_store.read(start->second);
                object_meta obj;
                zpp::bits::in{bytes.span(), zpp::bits::size4b{}}(obj)
                    .or_throw();
                objects.push_back(
                    {.name = start->first,
                     .last_modified = std::move(obj.last_modified),
                     .size = obj.addr.data_size()});
            }
        }

        return objects;
    }

    void insert_object(const std::string& key, address addr) {
        object_meta obj;
        obj.addr = std::move(addr);
        obj.last_modified = utc_time::clock::now();

        std::vector<char> bytes;
        /*
         * Note: obj needs to be passed as const here, otherwise ZPP will call
         * the de-serialization function and we will not write the data.
         */
        zpp::bits::out{bytes, zpp::bits::size4b{}}(std::as_const(obj))
            .or_throw();
        const auto index = m_data_store.post_write(bytes);

        m_transaction_log.append(key, index,
                                 transaction_log::operation::INSERT_START);
        // TODO: handle rollback in case something goes up in smoke during the
        // transaction
        m_data_store.apply_write();

        if (const auto it = m_object_ptrs.find(key); it != m_object_ptrs.end())
            [[unlikely]] {
            m_data_store.remove(it->second);
            it->second = index;
        } else [[likely]] {
            m_object_ptrs.emplace_hint(it, key, index);
        }

        m_transaction_log.append(key, index,
                                 transaction_log::operation::INSERT_END);
    }

    address get_obj(const std::string& key) {

        const auto it = m_object_ptrs.find(key);
        if (it == m_object_ptrs.end()) [[unlikely]] {
            throw error_exception(
                {error::object_not_found, "Attempt to get object '" + key +
                                              "' failed: no such object."});
        }

        const auto bytes = m_data_store.read(it->second);
        object_meta obj;
        zpp::bits::in{bytes.span(), zpp::bits::size4b{}}(obj).or_throw();

        return obj.addr;
    }

    void delete_object(const std::string& key) {
        if (const auto it = m_object_ptrs.find(key); it != m_object_ptrs.end())
            [[unlikely]] {
            m_transaction_log.append(key, it->second,
                                     transaction_log::operation::REMOVE_START);

            auto index = it->second;
            m_object_ptrs.erase(it);
            m_data_store.remove(index);
            m_transaction_log.append(key, index,
                                     transaction_log::operation::REMOVE_END);
        }
    }

    size_t get_used_space() const { return m_data_store.get_used_space(); }

    void destroy_bucket() { std::filesystem::remove_all(m_bucket_path); }

private:
    std::filesystem::path m_bucket_path;
    chaining_data_store m_data_store;
    transaction_log m_transaction_log;
    std::map<std::string, uint64_t> m_object_ptrs;
};

} // namespace uh::cluster

#endif // CORE_BUCKET_H
