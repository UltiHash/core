//
// Created by masi on 10/2/23.
//

#ifndef CORE_DIRECTORY_STORE_H
#define CORE_DIRECTORY_STORE_H

#include <unordered_map>
#include <string>
#include <utility>

#include "common/common.h"
#include "object_store.h"

namespace uh::cluster {

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
