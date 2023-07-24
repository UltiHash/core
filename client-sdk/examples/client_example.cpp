#include <iostream>
#include "../include/udb.h"
#include "string.h"

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

    enum operation_arguments
    {
        CLIENT_PUT,
        CLIENT_GET
    } operation_type;

    if (strcmp(argv[1], "put") == 0)
    {
        operation_type = CLIENT_PUT;
    }
    else
    {
        if (strcmp(argv[1], "get") == 0)
        {
            operation_type = CLIENT_GET;
        }
        else
        {
            std::cout << "error_occured: you need to specify either put or get in the first argument!"
                      << '\n';
            exit(1);
        }
    }

    /* key source argument 2 */

    char* key;
    long key_size = strlen(argv[2]);

    if (strrchr(argv[2], '/'))
    {
        FILE* fp = fopen(argv[2], "r");

        if (!fp)
        {
            std::cout << "error_occured: key file could not be opened!" << '\n';
            exit(1);
        }

        fseek(fp, 0L, SEEK_END);
        key_size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);

        key = (char*) malloc((key_size+1) * sizeof(char));
        fread(key, key_size, sizeof(char), fp);

        fclose(fp);
    }
    else
    {
        key = (char*) malloc((key_size+1) * sizeof(char));
        strcpy(key, argv[2]);
    }

    if (!key)
    {
        std::cout << "error_occured: key entered was not valid!" << '\n';
        exit(1);
    }

    /* case put argument 3, get data */
    char* source = nullptr;
    long source_size = 0;

    if (operation_type == CLIENT_PUT)
    {
        FILE* fp_put = fopen(argv[3], "r");

        if (!fp_put)
        {
            std::cout << "error_occured: value source file could not be opened!" << '\n';
            exit(1);
        }

        fseek(fp_put, 0L, SEEK_END);
        source_size = ftell(fp_put);
        fseek(fp_put, 0L, SEEK_SET);

        source = (char*) malloc((source_size+1) * sizeof(char));
        fread(source, source_size, sizeof(char), fp_put);

        fclose(fp_put);
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

    if (operation_type == CLIENT_PUT)
    {
        /* initialize object with key, value, and label */
        UDB_OBJECT* obj1 = udb_init_object(key, key_size, source, source_size,
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
        udb_read_query_add_key(test_read_query, key, key_size);

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

        FILE* write_fp = fopen(argv[3], "w");
        fwrite(result->value, result->value_size, sizeof(char), write_fp);
        fclose(write_fp);

        /* getting object */
        udb_destroy_read_query_results(results);
        udb_destroy_read_query(test_read_query);
    }

    /* initialization stuffs */
    udb_destroy_connection(udb_conn);
    udb_destroy_instance(udb);
    udb_destroy_config(udb_config);

    free(key);
    free(source);

    return 0;
}
