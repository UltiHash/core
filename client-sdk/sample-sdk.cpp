//#include <iostream>
//#include "../include/ultidb.h"
//
//int main()
//{
//     std::cout << "UDB version " << get_sdk_version() << "\n";
//
//     /* Initialization */
//
//         /* Get a pointer to the UDB Config File */
//        UDB_CONFIG* udb_config = udb_create_config();
//        if (udb_config == nullptr)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//        udb_config_set_host_node(udb_config, "localhost", 0x5548);
//
//        /* Create an instance of UDB using the config file */
//        UDB* udb = udb_create_instance(udb_config);
//        if (udb == nullptr)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//        /* Get a connection to the UDB */
//        UDB_CONNECTION* udb_conn = udb_create_connection(udb);
//        if (udb_conn == nullptr)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//        /* ping the connection */
//        if (udb_ping(udb_conn) != UDB_RESULT_SUCCESS)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//
//    /* For a single document */
//
//        /* random data */
//        char client_data[] ="Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut"
//                            " labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco "
//                            "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
//                            "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat"
//                            " non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
//
//        /* creating a udb_doc and a key structure */
//        UDB_DOCUMENT udb_doc(client_data, sizeof(client_data));
//
//        UDB_KEY* udb_key = udb_create_empty_key();
//
//        if (udb_add_one(udb_conn, &udb_doc, udb_key) != UDB_RESULT_SUCCESS)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//        std::cout << "\n\033[1;4mAdd a single document:\033[0m\n" << udb_key->key << '\n';
//
//        /* get a single document */
//        UDB_DOCUMENT* udb_empty_doc = udb_create_empty_document();
//
//        if (udb_get_one(udb_conn, udb_key, udb_empty_doc) != UDB_RESULT_SUCCESS)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//        std::cout << "\n\033[1;4mGet a single document:\033[0m\n" << udb_empty_doc->data << "\n\n";
//
//
//    /* For multiple documents */
//
//        /* some random data */
//        char test_data_1[] = "The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs. "
//                           "How quickly daft jumping zebras vex. Crazy Fredrick bought many very exquisite opal jewels. "
//                           "The five boxing wizards jump quickly. Mr. Jock, TV quiz PhD, bags few lynx. Sphinx of black "
//                           "quartz, judge my vow. Jackdaws love my big sphinx of quartz. Bright vixens jump; dozy fowl "
//                           "quack. Sphinx of black quartz, judge my vow. Waltz, nymph, for quick jigs vex Bud. How vexingly "
//                           "quick daft zebras jump! Quick zephyrs blow, vexing daft Jim. Cozy lummox gives smart squid "
//                           "who asks for job pen. A wizard’s job is to vex chumps quickly in fog. The quick brown fox "
//                           "jumps over the lazy dog.";
//
//        char test_data_2[] = "Jumbled quirky text with no apparent meaning. Zxvqwyj fmgpa uosm xad no edlkouo yzbrmyi. "
//                             "Pqnvwq zlxud xnbq tlz btvi gjpdqlbf asikqls qypfz mntuo. Hkqnyrd mdwfh pmqg eaua fgyvvb "
//                             "qzqweufi dtdqm gnbge. Icnvw cmhbq yvqf tkw dmtgs wtrgojdqz hggcqde apmdr. Kojzptz qbtsi "
//                             "mznq bvwu uybwcnc wulvzkki gppdm jqmn. Sobjp hfhaq kkih glvyzlj ewztnq yilxrswe nsqwtn "
//                             "epqq. Vtxh dgrlq byng itze adcsrogy zrizxsq ydnayb vtay. Vqthd jkbqx ksgo ykmvzz fljo "
//                             "kgnxfuab pdmgcw xsifx.";
//
//        char test_data_3[] = "1G3#%&6d2$!a9B7@4C5c8D0e#F!hIjKgMfLhNjPiQkOhRl#SpUtVrXqYsZtWu+v=x-wy/z";
//
//        /* create documents for the data */
//        UDB_DOCUMENT udb_doc_1(test_data_1, sizeof(test_data_1));
//        UDB_DOCUMENT udb_doc_2(test_data_2, sizeof(test_data_2));
//        UDB_DOCUMENT udb_doc_3(test_data_3, sizeof(test_data_3));
//
//        const UDB_DOCUMENT* udb_doc_container[] = {&udb_doc_1, &udb_doc_2, &udb_doc_3};
//
//        /* get keys from the document */
//        UDB_KEY* udb_key_container[sizeof(udb_doc_container) / sizeof(UDB_DOCUMENT*)];
//
//        if (udb_add(udb_conn, udb_doc_container, udb_key_container, sizeof(udb_doc_container)/ sizeof(UDB_DOCUMENT*)) != UDB_RESULT_SUCCESS)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//        std::cout << "\033[1;4mAdd Multiple Documents:\033[0m\n";
//        for (size_t index = 0; index < sizeof(udb_key_container) / sizeof(UDB_KEY*); index++)
//        {
//            std::cout << udb_key_container[index]->key << '\n';
//        }
//
//
//        /* retrieving multiple documents */
//
//        UDB_DOCUMENT* udb_empty_document_container[sizeof(udb_key_container)/sizeof(UDB_KEY*)];
//
//        if (udb_get(udb_conn, const_cast<const UDB_KEY**>(udb_key_container), udb_empty_document_container, sizeof(udb_key_container) / sizeof(UDB_KEY*)) != UDB_RESULT_SUCCESS)
//        {
//            std::cout << "error_occured: " << get_error_message();
//            exit(0);
//        }
//
//        std::cout << "\033[1;4mGet multiple documents\033[0m\n\n";
//        for (size_t index = 0; index < sizeof(udb_empty_document_container) / sizeof(UDB_DOCUMENT*); index++)
//        {
//            std::cout << udb_empty_document_container[index]->data << "\n\n";
//        }
//
//
//    /* cleanup */
//
//        /* single document */
//        udb_destroy_key(udb_key);
//        udb_destroy_document(udb_empty_doc);
//
//        /* multiple document */
//        udb_destroy_multiple_keys(const_cast<const UDB_KEY**>(udb_key_container), sizeof(udb_key_container)/sizeof(UDB_KEY*));
//        udb_destroy_multiple_documents(const_cast<const UDB_DOCUMENT**>(udb_empty_document_container), sizeof(udb_empty_document_container)/sizeof(UDB_DOCUMENT*));
//
//        udb_destroy_connection(udb_conn);
//        udb_destroy_instance(udb);
//        udb_destroy_config(udb_config);
//
//    return 0;
//}
