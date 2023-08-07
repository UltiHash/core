#include <iostream>
#include "../include/udb.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <ranges>
#include <algorithm>
#include <openssl/sha.h>

struct udb_init
{
public:
    UDB_CONFIG* udb_config;
    UDB* udb;
    UDB_CONNECTION* udb_conn;

    udb_init()
    {
        /* Initialization */

        /* Get a pointer to the UDB Config File */
        udb_config = udb_create_config();
        if (udb_config == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(1);
        }
        udb_config_set_host_node(udb_config, "localhost", 0x5548);// port 21832

        /* Create an instance of UDB using the config file */
        udb = udb_create_instance(udb_config);
        if (udb == nullptr)
        {
            std::cout << "error_occured: " << get_error_message() << '\n';
            exit(1);
        }

        /* Get a connection to the UDB */
        udb_conn = udb_create_connection(udb);
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
    }

    ~udb_init()
    {
        udb_destroy_connection(udb_conn);
        udb_destroy_instance(udb);
        udb_destroy_config(udb_config);
    }
};

void put(std::vector<char>& key_string, std::vector<char>& source, udb_init& udb)
{
/* labels */
    char test_label_1[] = "Col1";
    char* test_labels[] = {test_label_1};

    /* initialize object with key, value, and label */
    UDB_OBJECT* obj1 = udb_init_object(key_string.data(), key_string.size(), source.data(), source.size(),
                                       test_labels, sizeof(test_labels) / sizeof(char*));

    /* create a write query */
    UDB_WRITE_QUERY* test_write_query = udb_create_write_query();
    udb_write_query_add_object(test_write_query, obj1);

    /* add object to the database */
    UDB_WRITE_QUERY_RESULTS* write_results = udb_put(udb.udb_conn, test_write_query);
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

void get(std::vector<char>& key_string, std::vector<char>& source, udb_init& udb)
{
/* create a read query*/
    UDB_READ_QUERY* test_read_query = udb_create_read_query();
    udb_read_query_add_key(test_read_query, key_string.data(), key_string.size());

    /* getting an object from database */
    UDB_READ_QUERY_RESULTS* results = udb_get(udb.udb_conn, test_read_query);
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

    if (!result)
    {
        std::cout << "error: nothing finally read from db" << '\n';
        exit(1);
    }

    source.assign(result->value, result->value + result->value_size);

    /* getting object */
    udb_destroy_read_query_results(results);
    udb_destroy_read_query(test_read_query);
}

void read_file(std::vector<char>& read_to, const std::filesystem::path& p)
{
    std::ifstream read(p, std::ios_base::in);
    unsigned long file_size = std::filesystem::file_size(p);
    read_to.resize(file_size);
    read.read(read_to.data(), file_size);
    read.close();
}

enum store_type
{
    FILE_TYPE,
    DIRECTORY_TYPE,
    DIRECT_TYPE
};

void put_traverse(std::vector<char>& key_string, const std::filesystem::path& source_path, udb_init& udb)
{
    unsigned char SHA512_buffer[512];
    std::vector<char> source;
    std::vector<char> uhv;
    std::vector<char> SHA_key;

    if (std::filesystem::is_regular_file(source_path))
    {
        uhv.push_back(DIRECT_TYPE);

        read_file(source, source_path);

        SHA512(std::vector<unsigned char>(source.cbegin(), source.cend()).data(),
               source.size(),
               SHA512_buffer);

        SHA_key.assign(SHA512_buffer, SHA512_buffer + 512);

        put(SHA_key, source, udb);
        uhv.insert(uhv.cend(), SHA_key.cbegin(), SHA_key.cend());
    }
    else
    {
        for (auto const& dir_entry : std::filesystem::recursive_directory_iterator(source_path))
        {
            uhv.push_back((store_type) dir_entry.is_directory());

            auto dir_name = relative(dir_entry.path(), source_path).string();
            source.assign(dir_name.cbegin(), dir_name.cend());

            SHA512(std::vector<unsigned char>(dir_name.cbegin(), dir_name.cend()).data(),
                   dir_name.size(),
                   SHA512_buffer);

            SHA_key.assign(SHA512_buffer, SHA512_buffer + 512);

            put(SHA_key, source, udb);
            uhv.insert(uhv.cend(), SHA_key.cbegin(), SHA_key.cend());

            if (not dir_entry.is_directory())
            {
                read_file(source, dir_entry.path());

                SHA512(std::vector<unsigned char>(source.cbegin(), source.cend()).data(),
                       source.size(),
                       SHA512_buffer);

                SHA_key.assign(SHA512_buffer, SHA512_buffer + 512);
                put(SHA_key, source, udb);
                uhv.insert(uhv.cend(), SHA_key.cbegin(), SHA_key.cend());
            }
        }
    }
    put(key_string, uhv, udb);
}

void get_traverse(std::vector<char>& key_string, const std::filesystem::path& source_path, udb_init& udb)
{
    std::vector<char> source;
    std::vector<char> uhv;
    std::vector<char> SHA_key;
    unsigned long count{};

    get(key_string, uhv,udb);

    do{
        switch (uhv[count])
        {
            case FILE_TYPE:
            {
                count++;
                SHA_key.assign(uhv.cbegin()+count,uhv.cbegin()+count+512);

                get(SHA_key, source, udb);
                count+=512;

                std::filesystem::path out_path{source.cbegin(),source.cend()};
                out_path = source_path / out_path;

                SHA_key.assign(uhv.cbegin()+count,uhv.cbegin()+count+512);

                get(SHA_key, source, udb);
                count+=512;

                std::filesystem::create_directories(out_path.parent_path());
                std::ofstream write_stream(out_path, std::ios_base::out);

                write_stream.write(source.data(),source.size());
            }
            break;
            case DIRECTORY_TYPE:
            {
                count++;
                SHA_key.assign(uhv.cbegin()+count,uhv.cbegin()+count+512);

                get(SHA_key, source, udb);
                count+=512;

                std::filesystem::path out_path{source.cbegin(),source.cend()};
                out_path = source_path / out_path;

                std::filesystem::create_directories(out_path);
            }
            break;
            case DIRECT_TYPE:
            {
                count++;
                SHA_key.assign(uhv.cbegin()+count,uhv.cbegin()+count+512);

                get(SHA_key, source, udb);
                count+=512;

                std::filesystem::path out_path = source_path;

                std::filesystem::create_directories(out_path.parent_path());
                std::ofstream write_stream(out_path, std::ios_base::out);

                write_stream.write(source.data(),source.size());
            }
            break;
        }
    }while(count < uhv.size());

}

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

    std::vector<char> key_string;
    key_string.resize(strlen(argv[2]));
    key_string.assign(argv[2], argv[2] + strlen(argv[2]));

    std::filesystem::path source_path{std::string{argv[3], strlen(argv[3])}};

    if (operation_type_string != "put" and operation_type_string != "get")
    {
        std::cout << "error_occured: you need to specify either put or get in the first argument!"
                  << '\n';
        exit(1);
    }

    /* key source argument 2 if the key can be found as a file path*/
    auto key_path = std::filesystem::path(std::string(key_string.begin(), key_string.end()));

    if (exists(key_path))
    {
        read_file(key_string, key_path);
    }

    /* Initialization */
    auto udb = udb_init();

    /* integrate source or read from db to source */

    if (operation_type_string == "put")
    {
        put_traverse(key_string, source_path, udb);
    }
    else
    {
        get_traverse(key_string, source_path, udb);
    }

    return 0;
}
