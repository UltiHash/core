#include <iostream>
#include "udb.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

int main(int argc, const char* argv[])
{

    std::cout << get_sdk_name() << " " << get_sdk_version() << "\n\n";

    if (argc != 4)
    {
        std::cout
            << "error_occured: you need to specify 3 arguments: put/get, key/uhv file, file to integrate/to write to!"
            << '\n';
        exit(1);
    }

    /* operation type argument 1 */
    std::string operation_type_string{argv[1], strlen(argv[1])};
    std::string key_string{argv[2], strlen(argv[2])};
    std::filesystem::path source_path{std::string{argv[3], strlen(argv[3])}};

    if (operation_type_string != "put" and operation_type_string != "get")
    {
        std::cout << "error_occured: you need to specify either put or get in the first argument!"
                  << '\n';
        exit(1);
    }

    /* key source argument 2 */

    if (exists(std::filesystem::path(key_string)))
    {
        std::ifstream key_read(key_string,std::ios_base::in);
        unsigned long key_file_size = std::filesystem::file_size(key_string);
        key_string.resize(key_file_size);
        key_read.read(key_string.data(),key_file_size);
        key_read.close();
    }

    /* case put argument 3, get data */
    std::vector<char> source;

    if (operation_type_string == "put")
    {
        std::ifstream source_read(source_path,std::ios_base::in);
        unsigned long source_file_size = std::filesystem::file_size(source_path);
        source.resize(source_file_size);
        source_read.read(source.data(),source_file_size);
        source_read.close();
    }

    /* Initialization */

    /* Get a pointer to the UDB Config File */
    UDB_CONFIG* udb_config = udb_create_config();
    if (udb_config == nullptr)
    {
        std::cout << "error_occured: " << get_error_message();
        exit(1);
    }
    udb_config_set_host_node(udb_config, "localhost", 0x5548);// port 21832

    /* Create an instance of UDB using the config file */
    UDB* udb = udb_create_instance(udb_config);
    if (udb == nullptr)
    {
        std::cout << "error_occured: " << get_error_message() << '\n';
        exit(1);
    }

    /* Get a connection to the UDB */
    UDB_CONNECTION* udb_conn = udb_create_connection(udb);
    if (udb_conn == nullptr)
    {
        std::cout << "error_occured: " << get_error_message() << '\n';
        exit(1);
    }

    /* ping the connection */
    if (udb_ping(udb_conn) != UDB_RESULT_SUCCESS)
    {
        std::cout << "error_occured: " << get_error_message() << '\n';
        exit(1);
    }

    /* integrate source or read from db to source */

    /* labels */
    char test_label_1[] = "Col1";
    char* test_labels[] = {test_label_1};

    if (operation_type_string == "put")
    {
        /* initialize object with key, value, and label */
        UDB_OBJECT* obj1 = udb_init_object(key_string.data(), key_string.size(), source.data(), source.size(),
                                           test_labels, sizeof(test_labels) / sizeof(char*));

        /* create a write query */
        UDB_WRITE_QUERY* test_write_query = udb_create_write_query();
        udb_write_query_add_object(test_write_query, obj1);

        /* add object to the database */
        UDB_WRITE_QUERY_RESULTS* write_results = udb_put(udb_conn, test_write_query);
        if (write_results == nullptr)
        {
            std::cout << "error: " << get_error_message() << '\n';
            exit(1);
        }

        /* print all effective sizes */
        size_t eff_count = udb_get_effective_sizes_count(write_results);

        std::cout << "Effective Sizes: ";
        for (auto count = 0; count < eff_count; count++)
        {
            std::cout << udb_get_effective_size(write_results, count) << " ";
        }
        std::cout << "\n\n";

        int return_code_1 = udb_get_return_code(write_results, 0);
        std::cout << "Return code: " << return_code_1 << "\n\n";

        /* putting object */
        udb_destroy_write_query_results(write_results);
        udb_destroy_write_query(test_write_query);
        udb_destroy_object(obj1);
    }
    else
    {
        /* create a read query*/
        UDB_READ_QUERY* test_read_query = udb_create_read_query();
        udb_read_query_add_key(test_read_query, key_string.data(), key_string.size());

        /* getting an object from database */
        UDB_READ_QUERY_RESULTS* results = udb_get(udb_conn, test_read_query);
        if (results == nullptr)
        {
            std::cout << "error: " << get_error_message() << '\n';
            exit(1);
        }

        UDB_READ_QUERY_RESULT* result;
        if (udb_results_next(results, &result))
        {
            std::cout << "result retrieved" << '\n';
        }

        if(!result){
            std::cout << "error: nothing finally read from db" << '\n';
            exit(1);
        }

        std::ofstream write_back(source_path, std::ios_base::out);
        write_back.write(result->value,result->value_size);
        write_back.close();

        /* getting object */
        udb_destroy_read_query_results(results);
        udb_destroy_read_query(test_read_query);
    }

    /* initialization stuffs */
    udb_destroy_connection(udb_conn);
    udb_destroy_instance(udb);
    udb_destroy_config(udb_config);

    return 0;
}
