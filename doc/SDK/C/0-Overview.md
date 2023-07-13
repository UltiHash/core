# C SDK - Overview

## Version

To get the version and the name of the SDK being used, the following two functions can be used.

 - returns a `const char*` that points to the array containing the name of the SDK
```c++
const char* get_sdk_name();
```


 - returns a `const char*` that points to the array containing the version of the sdk

```c++
const char* get_sdk_version();
```

## Simple example

The following is a simple `main` program that uses the UltiHash C API to create some objects, write them and retrieve them. The rest of the documentation explain each of the API functions, but this document is a quick reference to kick-start the user. The basic steps to follow when working with UltiHash are:

1. UltiHash Initialization,
2. Adding `objects` to UltiHash,
3. Querying `objects` from UltiHash,
4. Session cleanup.



```c++
#include <iostream>
#include "../include/ultidb.h"
#include <cstring>

int main()
{
     std::cout << get_sdk_name() << " "  << get_sdk_version() << "\n\n";

     /* Initialization */

         /* Get a pointer to the UDB Config File */
        UDB_CONFIG* udb_config = udb_create_config();
        if (udb_config == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(1);
        }
        udb_config_set_host_node(udb_config, "localhost", 0x5548);

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

    /* some random data */

        /* data */
        char test_data_1[] = "The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs. "
                           "How quickly daft jumping zebras vex. Crazy Fredrick bought many very exquisite opal jewels. "
                           "The five boxing wizards jump quickly. Mr. Jock, TV quiz PhD, bags few lynx. Sphinx of black "
                           "quartz, judge my vow. Jackdaws love my big sphinx of quartz. Bright vixens jump; dozy fowl "
                           "quack. Sphinx of black quartz, judge my vow. Waltz, nymph, for quick jigs vex Bud. How vexingly "
                           "quick daft zebras jump! Quick zephyrs blow, vexing daft Jim. Cozy lummox gives smart squid "
                           "who asks for job pen. A wizard’s job is to vex chumps quickly in fog. The quick brown fox "
                           "jumps over the lazy dog.";

        char test_data_2[] = "Lorem Ipsum comes from a latin text written in 45BC by Roman statesman, lawyer, scholar, "
                             "and philosopher, Marcus Tullius Cicero. The text is titled `de Finibus Bonorum et Malorum`"
                             " which means `The Extremes of Good and Evil`. The most common form of Lorem ipsum is the "
                             "following: Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor."
                             "quack. Sphinx of black quartz, judge my vow. Waltz, nymph, for quick jigs vex Bud. How vexingly "
                             "quick daft zebras jump! Quick zephyrs blow, vexing daft Jim. Cozy lummox gives smart squid "
                             "who asks for job pen. A wizard’s job is to vex chumps quickly in fog. The quick brown fox "
                             "jumps over the lazy dog.";

        /* key */
        char test_key_1[] = "This_is_a_user_defined_key_1";
        char test_key_2[] = "This_is_a_user_defined_key_2";

        /* labels */
        char test_label_1[] = "Fox";
        char test_label_2[] = "Dog";
        char* test_labels[] = {test_label_1, test_label_2};

    /* adding objects to database */

        /* initialize object with key, value, and label */

        UDB_OBJECT* test_doc_1 = udb_init_object(test_key_1, strlen(test_key_1), test_data_1, strlen(test_data_1),
                                                     test_labels, sizeof(test_labels) / sizeof(char*));
        UDB_OBJECT* test_doc_2 = udb_init_object(test_key_2, strlen(test_key_2), test_data_2, strlen(test_data_2),
                                                 test_labels, sizeof(test_labels) / sizeof(char*));

    /* create a write query */
        UDB_WRITE_QUERY* test_write_query = udb_create_write_query();
        udb_write_query_add_object(test_write_query, test_doc_1);
        udb_write_query_add_object(test_write_query, test_doc_2);

        /* add object to the database */
        UDB_WRITE_QUERY_RESULTS* write_results = udb_put(udb_conn, test_write_query);
        if ( write_results == nullptr)
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
        int return_code_2 = udb_get_return_code(write_results, 1);
        std::cout << "Return code: " << return_code_1 << " " << return_code_2 << "\n\n";

    /* getting a objects from the database */

        /* create a read query*/
        UDB_READ_QUERY* test_read_query = udb_create_read_query();
        udb_read_query_add_key(test_read_query, test_key_1, strlen(test_key_1));
        udb_read_query_add_key(test_read_query, test_key_2, strlen(test_key_2));

        /* getting an object from database */
        UDB_READ_QUERY_RESULTS* results = udb_get(udb_conn, test_read_query);
        if (results == nullptr)
        {
            std::cout << "error: " << get_error_message() << '\n';
            exit(1);
        }

        UDB_READ_QUERY_RESULT* result;
        while (udb_results_next(results, &result))
        {
            std::cout << "Received Key:\n" << std::string_view (result->key, result->key_size) << "\n\n";
            std::cout << "Received Value:\n" << std::string_view (result->value, result->value_size) << "\n\n";
        }

    /* cleanup */

        /* getting object */
        udb_destroy_read_query_results(results);
        udb_destroy_read_query(test_read_query);

        /* putting object */
        udb_destroy_write_query_results(write_results);
        udb_destroy_write_query(test_write_query);
        udb_destroy_object(test_doc_1);

        /* initialization stuffs */
        udb_destroy_connection(udb_conn);
        udb_destroy_instance(udb);
        udb_destroy_config(udb_config);

    return 0;
}
```