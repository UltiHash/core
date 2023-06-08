//
// Created by masi on 5/30/23.
//

#ifndef CORE_SMART_CONFIG_H
#define CORE_SMART_CONFIG_H

#include <filesystem>
#include <list>

namespace uh::dbn::storage::smart {

struct map_config {
    unsigned long key_size;
    unsigned long map_key_file_init_size;
    unsigned long map_values_minimum_file_size;
    unsigned long map_values_maximum_file_size;
    double map_load_factor;
    unsigned long map_maximum_extension_factor;
    std::filesystem::path hashtable_key_path;
    std::filesystem::path hashtable_value_directory;
};

struct set_config {
    unsigned long set_init_file_size;
    unsigned long set_minimum_free_space;
    unsigned long max_empty_hole_size;
    std::filesystem::path fragment_set_path;

};

struct data_store_config {
    std::list <std::filesystem::path> data_store_files;
    unsigned long data_store_file_size;
};

struct dedupe_config {
    unsigned long min_fragment_size;
};

struct smart_config {
    map_config map_conf;
    set_config set_conf;
    data_store_config data_store_conf;
    dedupe_config dedupe_conf;
};

}

#endif //CORE_SMART_CONFIG_H
