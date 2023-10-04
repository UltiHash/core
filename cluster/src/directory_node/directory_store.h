//
// Created by masi on 10/2/23.
//

#ifndef CORE_DIRECTORY_STORE_H
#define CORE_DIRECTORY_STORE_H

#include <unordered_map>
#include <string>

#include "common/common.h"
#include "chaining_data_store.h"

namespace uh::cluster {

class log_file {
public:
    enum operation:uint8_t {
        INSERT_START,
        INSERT_END,
        DELETE_START,
        DELETE_END,
        UPDATE_START,

        INSERT,
    };

    void append (std::span <char> key, uint64_t object_id, operation op) {

    }

    std::unordered_map <std::string, uint64_t> replay () {

    }


    void recreate () {
        // not for now
    }
};

/*
 *
 *

 insert k1 v1
 insert k2 v2
 update start k1 v1
 update end k1 v3


 insert k1 v3
 insert k2 v2




 */


class object_store {

    explicit object_store (chaining_data_store_config& conf, std::unordered_map<std::string, uint64_t>& object_ptrs,
                           log_file& log_file):
        m_data_store (std::move(conf)), m_object_ptrs (object_ptrs), m_log_file (log_file) {}

    void insert (std::string key, std::span <char> data) {
        std::lock_guard <std::shared_mutex> lock (m_mutex);
        const auto index = m_data_store.post_write (data);
        m_log_file.append(key, index, log_file::operation::INSERT_START);
        m_data_store.apply_write();
        // TODO we don't need the lock anymore
        m_object_ptrs.insert({key, index});
        m_log_file.append(key, index, log_file::operation::INSERT_END);
        //todo: proper error handling
    }

    ospan <char> get (const std::string& key) {
        const auto index = m_object_ptrs.at(key);
        return m_data_store.read(index);
        //todo: proper error handling
    }

    private:
    chaining_data_store m_data_store;
    std::unordered_map<std::string, uint64_t> m_object_ptrs;
    log_file m_log_file;
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
