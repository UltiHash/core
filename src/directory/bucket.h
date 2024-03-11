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

    std::vector<object_meta> list_objects(const std::string& lower_bound,
                                          const std::string& prefix) {
        std::vector<std::string> keys;
        keys.reserve(m_object_ptrs.size());
        for (const auto& obj : m_object_ptrs) {
            keys.emplace_back(obj.first);
        }

        std::vector<std::string> filtered_keys;
        auto lower_bound_it =
            lower_bound.empty()
                ? keys.begin()
                : std::upper_bound(keys.begin(), keys.end(), lower_bound);
        std::copy_if(lower_bound_it, keys.end(),
                     std::back_inserter(filtered_keys),
                     [prefix](const std::string& key) {
                         return prefix.empty() || key.find(prefix) == 0;
                     });

        std::vector<object_meta> objects;
        for (auto& key : filtered_keys) {
            auto object_meta = m_object_meta[key];
            object_meta.name = key;
            objects.emplace_back(std::move(object_meta));
        }

        return objects;
    }

    void insert_object(const std::string& key, std::span<char> data,
                       std::size_t data_size) {
        const auto index = m_data_store.post_write(data);
        m_transaction_log.append(key, index,
                                 transaction_log::operation::INSERT_START);

        // TODO: handle rollback in case something goes up in smoke during the
        // transaction
        m_data_store.apply_write();
        m_object_ptrs[key] = index;
        if (const auto it = m_object_meta.find(key); it != m_object_meta.end())
            [[unlikely]] {
            it->second.last_modified = get_current_ISO8601_datetime();
            it->second.size = data_size;
        } else {
            m_object_meta[key] = {
                .created_date = get_current_ISO8601_datetime(),
                .last_modified = get_current_ISO8601_datetime(),
                .size = data_size};
        }

        m_transaction_log.append(key, index,
                                 transaction_log::operation::INSERT_END);
    }

    unique_buffer<char> get_obj(const std::string& key) {

        const auto it = m_object_ptrs.find(key);
        if (it == m_object_ptrs.end()) [[unlikely]] {
            throw error_exception(
                {error::object_not_found, "Attempt to get object '" + key +
                                              "' failed: no such object."});
        }

        return m_data_store.read(it->second);
    }

    void delete_object(const std::string& key) {
        if (const auto it = m_object_ptrs.find(key); it != m_object_ptrs.end())
            [[unlikely]] {
            m_transaction_log.append(key, it->second,
                                     transaction_log::operation::REMOVE_START);

            auto index = it->second;
            m_object_ptrs.erase(it);
            m_object_meta.erase(key); // Perhaps this can be merged together
                                      // with the object_ptrs?
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
    std::map<std::string, object_meta> m_object_meta;
};

} // namespace uh::cluster

#endif // CORE_BUCKET_H
