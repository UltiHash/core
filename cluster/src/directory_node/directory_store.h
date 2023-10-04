//
// Created by masi on 10/2/23.
//

#ifndef CORE_DIRECTORY_STORE_H
#define CORE_DIRECTORY_STORE_H

#include <unordered_map>
#include <string>
#include <utility>

#include "common/common.h"
#include "chaining_data_store.h"
#include "transaction_log.h"

namespace uh::cluster {

class object_store {

    explicit object_store (chaining_data_store_config& conf, std::filesystem::path log_path,
                           std::unordered_map<std::string, uint64_t>& object_ptrs):
        m_data_store (std::move(conf)), m_log_file (std::move(log_path)), m_object_ptrs (object_ptrs) {}

    void insert (const std::string& key, std::span <char> data) {
        if(m_object_ptrs.contains(key)) [[unlikely]] {
            throw std::runtime_error ("Attempt to insert object ' + key + ' failed: an object with the same name already exists.");
        }
        std::lock_guard <std::shared_mutex> lock (m_mutex);
        const auto index = m_data_store.post_write (data);
        m_log_file.append(key, index, transaction_log::operation::INSERT_START);
        //TODO: handle rollback in case something goes up in smoke during the transaction
        m_data_store.apply_write();
        m_object_ptrs[key] = index;
        m_log_file.append(key, index, transaction_log::operation::INSERT_END);
    }

    ospan <char> get (const std::string& key) {
        if(!m_object_ptrs.contains(key)) [[unlikely]] {
            throw std::runtime_error ("Attempt to get object ' + key + ' failed: no such object.");
        }
        const auto index = m_object_ptrs.at(key);
        return m_data_store.read(index);
    }

    void remove (const std::string& key) {
        if(!m_object_ptrs.contains(key)) [[unlikely]] {
            throw std::runtime_error ("Attempt to remove object ' + key + ' failed: no such object.");
        }
        const auto index = m_object_ptrs.at(key);
        m_log_file.append(key, index, transaction_log::operation::REMOVE_START);
        m_object_ptrs.erase(key);
        m_data_store.remove(index);
        m_log_file.append(key, index, transaction_log::operation::REMOVE_END);
    }

    void update (const std::string& key, std::span <char> data) {
        //TODO: consider dropping update (as S3 has no update) and merge the functionality with insert
        std::lock_guard <std::shared_mutex> lock (m_mutex);
        const auto index = m_data_store.post_write (data);
        m_log_file.append(key, index, transaction_log::operation::UPDATE_START);
        m_data_store.apply_write();
        const auto old_index = m_object_ptrs.at(key);
        m_data_store.remove(old_index);
        m_object_ptrs[key] = index;
        m_log_file.append(key, index, transaction_log::operation::UPDATE_END);
    }


    private:
    chaining_data_store m_data_store;
    transaction_log m_log_file;
    std::unordered_map<std::string, uint64_t> m_object_ptrs;
    std::shared_mutex m_mutex;
};

class bucket {
    object_store m_object_store;
    std::string bucket_name;
    // acl
};

class directory_store {

    std::unordered_map <std::string, bucket> m_buckets;

    // log file bucket_name

    void insert (const std::string& bucket, const std::string& key, const address& addr);
    address get (const std::string& bucket, const std::string& key);
    void add_bucket (const std::string& bucket);
    std::vector <std::string> list_keys (const std::string& bucket);
    std::vector <std::string> list_buckets (const std::string& bucket);
};

}
#endif //CORE_DIRECTORY_STORE_H
