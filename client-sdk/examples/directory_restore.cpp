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
#include <unordered_set>

int main(int argc, char* args[]) {
    std::cout << get_sdk_name() << " " << get_sdk_version() << "\n";

    std::filesystem::path out_path = "out_path";

    std::filesystem::create_directories(out_path);
    if (argc != 2) {
        std::cout << "Please provide a directory path and a key file" << std::endl;
    }

    std::filesystem::path path (args[1]);
    if (!std::filesystem::exists(path)) {
        std::cout << "The given key path \"" << path <<"\" is not a valid path" << std::endl;
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

    size_t max_val_size = 10ul * 1024ul * 1024ul;

    UDB_READ_QUERY* read_query = udb_create_read_query();
    std::unordered_set <std::filesystem::path> file_path_set;
    std::unordered_set <UDB_DATA*> key_set;

    std::fstream key_file (path, std::ios::in);

    std::filesystem::path root;
    key_file >> root;
    std::filesystem::path file_path;

    while (key_file >> file_path) {

        auto res = file_path_set.emplace(std::move (file_path));

        UDB_DATA* key = new UDB_DATA (const_cast <char*> (res.first->c_str()), res.first->string().size());
        key_set.emplace(key);
        udb_read_query_add_key(read_query, key);
    }

    UDB_DOCUMENTS* documents_container = udb_create_documents_container();

    std::chrono::time_point <std::chrono::steady_clock> timer;

    const auto before = std::chrono::steady_clock::now ();
    if (udb_get(udb_conn, read_query, documents_container) != UDB_RESULT_SUCCESS)
    {
        std::cout << "error: " << get_error_message();
        exit(-1);
    }

    const auto after = std::chrono::steady_clock::now ();
    const std::chrono::duration <double> duration = after - before;

    size_t total_size = 0;
    for (int i = 0; i < udb_get_documents_count(documents_container); i++) {
        UDB_DOCUMENT* retrieved_document = udb_get_document(documents_container, i);
        UDB_DATA* returned_value = udb_get_value(retrieved_document);
        UDB_DATA* returned_key = udb_get_key(retrieved_document);

        std::filesystem::path path {std::string_view (returned_key->data, returned_key->size)};
        const auto p = relative(path, root);
        const auto sp = out_path/ p;
        std::filesystem::create_directories(sp.parent_path());
        std::fstream new_file (sp, std::ios::trunc | std::ios::out | std::ios::binary);
        new_file.write(returned_value->data, returned_value->size);
        total_size += returned_value->size;
    }


    const auto total_size_gb = static_cast <double> (total_size) / (1024.0 * 1024.0 * 1024.0);
    const auto bw = total_size_gb / duration.count();

    // TODO how to get the dedupe ratio?
    std::cout << "Restored data of size " << total_size_gb << " with the bandwidth of " << bw << " GB/s" << std::endl;


    /* cleanup
    udb_destroy_documents_container(&documents_container);
    udb_destroy_read_query(&read_query);
    udb_destroy_connection(udb_conn);
    udb_destroy_instance(udb);
    udb_destroy_config(udb_config);
*/
}