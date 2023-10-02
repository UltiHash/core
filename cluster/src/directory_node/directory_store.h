//
// Created by masi on 10/2/23.
//

#ifndef CORE_DIRECTORY_STORE_H
#define CORE_DIRECTORY_STORE_H

#include "unordered_map"
#include "common/common.h"
#include "string"
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

    std::unordered_map <ospan<char>, uint64_t> replay () {

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
    std::unordered_map<ospan <char>, uint64_t> objects;
    chaining_data_store data_store;
    log_file m_log_file;
    void integrate (std::span <char> key, std::span <char> data) {
        const auto id = data_store.write (data);

        // write key + id into log
        //objects.insert (key, id);


        try {
            m_log_file.append(key, 0, log_file::operation::INSERT_START);
            //insertion
            //log.write

            //...
            m_log_file.append(key, 0, log_file::operation::INSERT_END);

        }catch (...) {
        }

    }
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
