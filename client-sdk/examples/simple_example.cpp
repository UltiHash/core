#include <iostream>
#include "../include/ultidb.h"
#include <string.h>

int main()
{
     std::cout << get_sdk_name() << " "  << get_sdk_version() << "\n\n";

     /* Initialization */

         /* Get a pointer to the UDB Config File */
        UDB_CONFIG* udb_config = udb_create_config();
        if (udb_config == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(-1);
        }
        udb_config_set_host_node(udb_config, "localhost", 0x5548);

        /* Create an instance of UDB using the config file */
        UDB* udb = udb_create_instance(udb_config);
        if (udb == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(-1);
        }

        /* Get a connection to the UDB */
        UDB_CONNECTION* udb_conn = udb_create_connection(udb);
        if (udb_conn == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(-1);
        }

        /* ping the connection */
        if (udb_ping(udb_conn) != UDB_RESULT_SUCCESS)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(-1);
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
        /* key */
        char test_key[] = "This_is_a_user_defined_key";

        /* labels */
        char test_label_1[] = "Fox";
        char test_label_2[] = "Dog";
        char* test_labels[] = {test_label_1, test_label_2};

    /* adding documents to database */

        /* initialize document with key, value, and label */

        UDB_DOCUMENT* test_doc_1 = udb_init_document(test_key, strlen(test_key), test_data_1, strlen(test_data_1),
                                                     test_labels, sizeof(test_labels) / sizeof(char*));

        /* create a write query */
        UDB_WRITE_QUERY* test_write_query = udb_create_write_query();
        udb_write_query_add_document(test_write_query, test_doc_1);

        /* add document to the database */
        UDB_WRITE_QUERY_RESULTS* write_results = udb_put(udb_conn, test_write_query);
        if ( write_results == nullptr)
        {
            std::cout << "error: " << get_error_message();
            exit(-1);
        }
        size_t count;
        uint32_t eff_size;
        udb_get_effective_sizes_count(write_results, &count);
        udb_get_effective_size(write_results, &eff_size, 0);

    /* getting a documents from the database */

        /* create a read query*/
        UDB_READ_QUERY* test_read_query = udb_create_read_query();
        udb_read_query_add_key(test_read_query, test_key, sizeof(test_key));

        /* getting a document from database */
        UDB_READ_QUERY_RESULTS* results = udb_get(udb_conn, test_read_query);
        if (results == nullptr)
        {
            std::cout << "error: " << get_error_message();
            exit(-1);
        }

        UDB_READ_QUERY_RESULT* result;
        while (udb_results_next(results, &result))
        {
            // use the ptr to print the result as result points to UDB_READ_QUERY_RESULT struct
        }

    /* cleanup */

        /* getting document */
        udb_destroy_read_query_results(&results);
        udb_destroy_read_query(&test_read_query);

        /* putting document */
        udb_destroy_write_query_results(&write_results);
        udb_destroy_write_query(&test_write_query);
        udb_destroy_document(&test_doc_1);

        /* initialization stuffs */
        udb_destroy_connection(udb_conn);
        udb_destroy_instance(udb);
        udb_destroy_config(udb_config);

    return 0;
}
