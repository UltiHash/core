//
// Created by masi on 6/27/23.
//
#include <iostream>
#include "../include/ultidb.h"
#include <string.h>
#include <filesystem>
#include <fstream>
#include <util/ospan.h>
#include <chrono>
#include <map>
#include <unordered_map>

size_t max_val_size = 500ul * 1024ul * 1024ul;


int main(int argc, char* args[]) {
    std::cout << get_sdk_name() << " " << get_sdk_version() << "\n";

    if (argc != 3) {
        std::cout << "Please provide a directory path and a key file" << std::endl;
    }

    std::filesystem::path path (args[1]);
    if (!std::filesystem::exists(path)) {
        std::cout << "The given path \"" << path <<"\" is not a valid path" << std::endl;
    }

    std::fstream key_file (args[2], std::ios::trunc | std::ios::out);
    if (!key_file.is_open()) {
        std::cout << "The given key path is not a valid path" << std::endl;
        exit(-1);
    }

    /* Initialization */

    /* Get a pointer to the UDB Config File */
    UDB_CONFIG *udb_config = udb_create_config();
    if (udb_config == nullptr) {
        std::cout << "error_occured: " << get_error_message();
        exit(-1);
    }
    udb_config_set_host_node(udb_config, "localhost", 0x5548);

    /* Create an instance of UDB using the config file */
    UDB *udb = udb_create_instance(udb_config);
    if (udb == nullptr) {
        std::cout << "error_occured: " << get_error_message();
        exit(-1);
    }

    /* Get a connection to the UDB */
    UDB_CONNECTION *udb_conn = udb_create_connection(udb);
    if (udb_conn == nullptr) {
        std::cout << "error_occured: " << get_error_message();
        exit(-1);
    }

    /* ping the connection */
    if (udb_ping(udb_conn) != UDB_RESULT_SUCCESS) {
        std::cout << "error_occured: " << get_error_message();
        exit(-1);
    }


    UDB_WRITE_QUERY* write_query = udb_create_write_query();
    size_t total_size = 0;
    std::map <std::string, uh::util::ospan <char>> file_data;
    std::unordered_map <UDB_DATA*, UDB_DATA*> key_value;
    key_file << path << std::endl;

    for (const std::filesystem::directory_entry &dir_entry: std::filesystem::recursive_directory_iterator(path)) {

        if (dir_entry.is_directory()) {
            continue;
        }

        const auto file_size = std::filesystem::file_size(dir_entry.path());
        if (file_size > max_val_size or file_size == 0) {
            std::cout << "ignoring file " << dir_entry.path()
                      << " with size << " << file_size
                      << " which is not in the range of allowed value sizes to maximum of "
                      << max_val_size << std::endl;
            continue;
        }

        key_file << dir_entry.path() << std::endl;

        std::fstream file(dir_entry.path());
        uh::util::ospan<char> data(file_size);
        file.read(data.data.get(), file_size);

        // TODO: i had to do const cast
        // TODO: maybe copy the key? because it gets destructed

        auto res = file_data.emplace(std::move(dir_entry.path()), std::move(data));
        UDB_DATA *key = new UDB_DATA(const_cast <char *> (res.first->first.c_str()),
                                     res.first->first.size());
        UDB_DATA *value = new UDB_DATA(res.first->second.data.get(), res.first->second.size);

        auto kv = key_value.emplace(key, value);

        UDB_DOCUMENT *doc = udb_init_document(kv.first->first, kv.first->second, nullptr, 0);
        udb_write_query_add_document(write_query, doc);

        total_size += file_size;
    }

    std::chrono::time_point <std::chrono::steady_clock> timer;
    std::cout << "total size is " << total_size << std::endl;

    const auto before = std::chrono::steady_clock::now ();
    // TODO udb_add maybe a better name?
    if (udb_add(udb_conn, write_query) != UDB_RESULT_SUCCESS)
    {
        std::cout << "error: " << get_error_message();
        exit(-1);
    }


    const auto after = std::chrono::steady_clock::now ();
    const std::chrono::duration <double> duration = after - before;

    const auto total_size_gb = static_cast <double> (total_size) / (1024.0 * 1024.0 * 1024.0);
    const auto bw = total_size_gb / duration.count();

    // TODO how to get the dedupe ratio?
    std::cout << "Stored data of size " << total_size_gb << " with the dedupe ratio of <not available> and the bandwidth of " << bw << " GB/s" << std::endl;

    double total_duration = 0;
    for (const std::filesystem::directory_entry &dir_entry: std::filesystem::recursive_directory_iterator(path)) {
        const auto file_size = std::filesystem::file_size(dir_entry.path());

        std::fstream file(dir_entry.path());
        uh::util::ospan<char> data(file_size);
        file.read(data.data.get(), file_size);

        const auto beforew = std::chrono::steady_clock::now ();

        std::fstream wfile ("tmp", std::ios::out | std::ios::trunc);
        wfile.write(data.data.get(), file_size);
        const auto afterw = std::chrono::steady_clock::now ();
        const std::chrono::duration <double> durationw = afterw - beforew;
        total_duration += durationw.count();
    }

    const auto rawbw = total_size_gb / total_duration;

    std::cout << "raw write bw " << rawbw << "GB/s" << std::endl;

    /* cleanup */
    udb_destroy_write_query(&write_query);
    udb_destroy_connection(udb_conn);
    udb_destroy_instance(udb);
    udb_destroy_config(udb_config);
}