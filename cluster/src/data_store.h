//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "cluster_config.h"
#include "common.h"
#include "free_spot_manager.h"
#include "ospan.h"
#include <span>
#include <memory_resource>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>

namespace uh::cluster {


class data_store {

public:

    explicit data_store (int id, data_store_config conf);

    address write (std::span <char> data);

    ospan <char> read (uint128_t pointer, size_t size) const;

    void remove (uint128_t pointer, size_t size);

    void sync ();

    uint128_t used_space ();

    ~data_store();


private:

    struct partial_alloc_t {
        int fd {};
        std::int64_t offset {};
        std::size_t size {};
        uint128_t global_offset;
    };
    typedef std::forward_list <partial_alloc_t> alloc_t;

    [[nodiscard]] std::pair <int, long> get_file_offset_pair (uint128_t pointer) const;

    alloc_t allocate (std::size_t size);

    int add_new_file (const uint128_t& offset, long file_size);

    [[nodiscard]] static std::pair <int, uint128_t> parse_file_name (const std::string& filename);

    [[nodiscard]] std::string get_name (const uint128_t& offset) const;

    const int m_id;
    const data_store_config m_conf;
    free_spot_manager m_free_spot_manager;
    std::map <uint128_t, int> m_open_files;
    std::forward_list <int> m_modified_files;
    std::unordered_map <int, std::size_t> m_file_data_sizes;
    std::unordered_map <int, std::size_t> m_file_sizes;
    std::shared_mutex m;

};

} // end namespace uh::cluster

#endif //CORE_DATA_STORE_H
