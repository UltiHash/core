//
// Created by masi on 10/2/23.
//

#ifndef CORE_DIRECTORY_STORE_H
#define CORE_DIRECTORY_STORE_H

#include <unordered_map>

#include <string>
#include <utility>

#include "common/common.h"
#include "bucket.h"

namespace uh::cluster {

class directory_store {

    std::unordered_map <std::string, std::unique_ptr <bucket>> m_buckets;
    std::filesystem::path m_root;
    bucket_config m_bucket_conf;

public:

    explicit directory_store (directory_store_config conf):
        m_root (std::move (conf.root)),
        m_bucket_conf (std::move (conf.bucket_conf))
    {
        std::filesystem::create_directories (m_root);
        for (const auto& entry: std::filesystem::directory_iterator (m_root)) {
            m_buckets.emplace (entry.path().filename(), std::make_unique <bucket> (m_root, entry.path().filename(), m_bucket_conf));
        }
    }

    void insert (const std::string& bucket, const std::string& key, const std::span <char>& data) {
        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend()) [[likely]] {
            b->second->insert_object(key, data);
        }
        else {
            throw std::out_of_range ("The bucket " + bucket + " does not exist.");
        }
    }

    ospan <char> get (const std::string& bucket, const std::string& key) {
        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend()) [[likely]] {
            return b->second->get_obj(key);
        }
        else {
            throw std::out_of_range ("The bucket " + bucket + " does not exist.");
        }
    }

    void add_bucket (const std::string& bucket_id) {
        m_buckets.emplace (bucket_id, std::make_unique <bucket> (m_root, bucket_id, m_bucket_conf));
    }

    void remove_object (const std::string& bucket_id, const std::string& object_key) {
        if (const auto& b = m_buckets.find(bucket_id); b != m_buckets.cend()) [[likely]] {
            b->second->delete_object(object_key);
        }
        else {
            throw error_exception(error::bucket_not_found);
        }
    }

    void remove (const std::string& bucket, const std::string& key) {
        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend()) [[likely]] {
            b->second->delete_object(key);
        }
        else {
            throw std::out_of_range ("The bucket " + bucket + " does not exist.");
        }
    }

    void remove_bucket (const std::string& bucket) {
        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend()) [[likely]]
        {
            if (b->second->is_empty())
            {
                b->second->destroy_bucket();
                m_buckets.erase(bucket);
            }
            else
            {
                throw error_exception (error::bucket_not_empty);
            }
        }
        else
        {
            throw error_exception(error::bucket_not_found);
        }
    }

    std::vector <std::string> list_keys (const std::string& bucket, const std::string& lower_bound = "", const std::string& prefix = "") {
        if (const auto& b = m_buckets.find(bucket); b != m_buckets.cend()) [[likely]] {
            return b->second->list_keys(lower_bound, prefix);
        }
        else {
            throw std::out_of_range ("The bucket " + bucket + " does not exist.");
        }
    }

    std::vector <std::string> list_buckets () {
        std::vector <std::string> buckets;
        buckets.reserve(m_buckets.size());
        for (const auto& bucket_name: m_buckets) {
            buckets.emplace_back (bucket_name.first);
        }
        return buckets;
    }

    size_t get_used_space () {
        size_t used_space = 0;
        for (const auto& bucket: m_buckets) {
            used_space += bucket.second->get_used_space();
        }
        return used_space;
    }
};

}
#endif //CORE_DIRECTORY_STORE_H
