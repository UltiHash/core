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
        UDB_DATA key_wrapper(test_key, strlen(test_key));
        UDB_DATA data_wrapper(test_data_1, strlen(test_data_1));
        UDB_DOCUMENT* test_doc_1 = udb_init_document( &key_wrapper, &data_wrapper,
                                                     test_labels, sizeof(test_labels) / sizeof(char*));

        /* create write query */
        UDB_WRITE_QUERY* test_write_query = udb_create_write_query();
        udb_write_query_add_document(test_write_query, test_doc_1);

        /* add document to the database */
        if (udb_add(udb_conn, test_write_query) != UDB_RESULT_SUCCESS)
        {
            std::cout << "error: " << get_error_message();
            exit(-1);
        }

    /* getting a documents from the database */

        /* create a container to hold the documents retrieved */
        UDB_DOCUMENTS* documents_container = udb_create_documents_container();

        /* create read query with keys */
        UDB_READ_QUERY* test_read_query = udb_create_read_query();
        udb_read_query_add_key(test_read_query, &key_wrapper);

        /* get documents from the database */
        if (udb_get(udb_conn, test_read_query, documents_container) != UDB_RESULT_SUCCESS)
        {
            std::cout << "error: " << get_error_message();
            exit(-1);
        }

        /* print the returned data */
        UDB_DOCUMENT* retrieved_document = udb_get_document(documents_container, 0);
        UDB_DATA* returned_value = udb_get_value(retrieved_document);
        UDB_DATA* returned_key = udb_get_key(retrieved_document);
        std::string key_string;
        std::string data_string;

        for (size_t i=0; i< returned_key->size; i++)
        {
            key_string.push_back(returned_key->data[i]);
        }
        for (size_t i=0; i< returned_value->size; i++)
        {
            data_string.push_back(returned_value->data[i]);
        }

        std::cout << "received key:\n" << key_string << "\n\n";
        std::cout << "data:\n" << data_string << "\n";

    /* cleanup */
        udb_destroy_documents_container(&documents_container);
        udb_destroy_read_query(&test_read_query);
        udb_destroy_write_query(&test_write_query);
        udb_destroy_connection(udb_conn);
        udb_destroy_instance(udb);
        udb_destroy_config(udb_config);

return 0;
}
