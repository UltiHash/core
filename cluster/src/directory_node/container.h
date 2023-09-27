//
// Created by masi on 9/25/23.
//

#ifndef CORE_CONTAINER_H
#define CORE_CONTAINER_H

#include <string>
#include <common/common_types.h>

namespace uh::cluster {

class container {
public:
    void insert (const std::string& bucket, const std::string& key, const address& addr);
    address get (const std::string& bucket, const std::string& key);
    std::vector <std::string> list_keys (const std::string& bucket);
private:
    // indirect_k_direct_v_map bucket_to_hashmap_id
    // std::vector <indirect_kv_map> hashmaps of key_to_objects
};

} // end namespace uh::cluster

#endif //CORE_CONTAINER_H
