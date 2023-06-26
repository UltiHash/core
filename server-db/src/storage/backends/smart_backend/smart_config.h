//
// Created by masi on 5/30/23.
//

#ifndef CORE_SMART_CONFIG_H
#define CORE_SMART_CONFIG_H

#include <filesystem>
#include <list>

namespace uh::dbn::storage::smart {

// storage types
struct growing_managed_storage_config {
    std::filesystem::path directory;
    std::filesystem::path log_file;
    size_t min_file_size;
    size_t max_file_size;
};

struct fixed_managed_storage_config {
    std::list <std::filesystem::path> data_store_files;
    std::filesystem::path log_file;
    unsigned long data_store_file_size;
};

struct growing_plain_storage_config {
    std::filesystem::path file;
    size_t init_size;
};

// data structures

struct set_config {
    unsigned long set_minimum_free_space;
    unsigned long max_empty_hole_size;
    growing_plain_storage_config key_store_config;
};

struct hashmap_config {
    unsigned long key_size;
    double map_load_factor;
    unsigned long map_maximum_extension_factor;
    growing_managed_storage_config value_store_config;
    growing_plain_storage_config key_store_config;
};

struct sorted_map_config {
    set_config index_store;
    growing_managed_storage_config data_store;
};

// deduplication
struct dedupe_config {
    unsigned long min_fragment_size;
};

struct smart_config {
    hashmap_config hashmap_key_store_conf;
    set_config fragment_set_conf;
    sorted_map_config sorted_key_store_config;
    fixed_managed_storage_config data_store_conf;
    dedupe_config dedupe_conf;
    size_t number_of_threads = DEFAULT_THREADS;
    static constexpr unsigned DEFAULT_THREADS = 5u;
};

}

#endif //CORE_SMART_CONFIG_H
