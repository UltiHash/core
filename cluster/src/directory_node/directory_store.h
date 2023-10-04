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

    directory_store (std::filesystem::path root, bucket_config bucket_conf):
        m_root (std::move (root)),
        m_bucket_conf (std::move (bucket_conf))
    {
        std::filesystem::create_directories (m_root);
        for (const auto& entry: std::filesystem::directory_iterator (m_root)) {
            m_buckets.emplace (entry.path().filename(), std::make_unique <bucket> (m_root, entry.path().filename(), m_bucket_conf));
        }
    }

    void insert (const std::string& bucket, const std::string& key, const std::span <char>& data) {
        m_buckets.at(bucket)->insert_object (key, data);
    }

    ospan <char> get (const std::string& bucket, const std::string& key) {
        return m_buckets.at(bucket)->get_obj(key);
    }

    void add_bucket (const std::string& bucket_id) {
        m_buckets.emplace (bucket_id, std::make_unique <bucket> (m_root, bucket_id, m_bucket_conf));
    }

    void remove (const std::string& bucket, const std::string& key) {
        m_buckets.at(bucket)->delete_object(key);
    }

    void remove_bucket (const std::string& bucket) {
        m_buckets.at(bucket)->destroy_bucket();
        m_buckets.erase(bucket);
    }

    std::vector <std::string> list_keys (const std::string& bucket) {
        return m_buckets.at (bucket)->list_keys();
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
