#ifndef CORE_DIRECTORY_STORE_H
#define CORE_DIRECTORY_STORE_H

#include <unordered_map>

#include <string>
#include <utility>

#include "bucket.h"
#include "common/registry/service_registry.h"
#include "common/utils/common.h"

namespace uh::cluster {

class directory_store {

    std::unordered_map<std::string, std::unique_ptr<bucket>> m_buckets;
    directory_store_config m_conf;
    std::filesystem::path m_root_dir;
    std::mutex m_mutex;

public:
    explicit directory_store(directory_store_config conf)
        : m_conf(std::move(conf)),
          m_root_dir(m_conf.working_dir / "buckets") {
        std::filesystem::create_directories(m_root_dir);
        for (const auto& entry :
             std::filesystem::directory_iterator(m_root_dir)) {
            m_buckets.emplace(entry.path().filename(),
                              std::make_unique<bucket>(m_root_dir,
                                                       entry.path().filename(),
                                                       m_conf.bucket_conf));
        }
    }

    void insert(const std::string& bucket, const std::string& key,
                address addr) {

        std::lock_guard<std::mutex> lock(m_mutex);

        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend())
            [[likely]] {
            b->second->insert_object(key, std::move(addr));
        } else {
            throw error_exception(error::bucket_not_found);
        }
    }

    void bucket_exists(const std::string& bucket) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_buckets.find(bucket) == m_buckets.end()) {
            throw error_exception(error::bucket_not_found);
        }
    }

    address get(const std::string& bucket, const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend())
            [[likely]] {
            return b->second->get_obj(key);
        } else {
            throw error_exception(error::bucket_not_found);
        }
    }

    void add_bucket(const std::string& bucket_id) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_buckets.emplace(bucket_id,
                          std::make_unique<bucket>(m_root_dir, bucket_id,
                                                   m_conf.bucket_conf));
    }

    void remove_object(const std::string& bucket_id,
                       const std::string& object_key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (const auto& b = m_buckets.find(bucket_id); b != m_buckets.cend())
            [[likely]] {
            b->second->delete_object(object_key);
        } else {
            throw error_exception(error::bucket_not_found);
        }
    }

    void remove_bucket(const std::string& name) {

        std::lock_guard<std::mutex> lock(m_mutex);

        auto b = m_buckets.find(name);

        if (b == m_buckets.end()) {
            throw error_exception(error::bucket_not_found);
        }

        auto& bucket = *b->second;

        for (const auto& object : bucket.list_objects("", "")) {
            bucket.delete_object(object.name);
        }

        bucket.destroy_bucket();
        m_buckets.erase(b);
    }

    std::vector<object> list_objects(const std::string& bucket,
                                     const std::string& lower_bound = "",
                                     const std::string& prefix = "") {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend())
            [[likely]] {
            return b->second->list_objects(lower_bound, prefix);
        } else {
            throw error_exception(error::bucket_not_found);
        }
    }

    std::vector<std::string> list_buckets() {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<std::string> buckets;
        buckets.reserve(m_buckets.size());
        for (const auto& bucket_name : m_buckets) {
            buckets.emplace_back(bucket_name.first);
        }
        return buckets;
    }

    size_t get_used_space() {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t used_space = 0;
        for (const auto& bucket : m_buckets) {
            used_space += bucket.second->get_used_space();
        }
        return used_space;
    }
};

} // namespace uh::cluster
#endif // CORE_DIRECTORY_STORE_H
