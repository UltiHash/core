#include <iostream>
#include "../include/ultidb.h"
#include <string.h>

int main()
{
     std::cout << "UDB version " << get_sdk_version() << "\n";

     /* Initialization */

         /* Get a pointer to the UDB Config File */
        UDB_CONFIG* udb_config = udb_create_config();
        if (udb_config == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(0);
        }
        udb_config_set_host_node(udb_config, "localhost", 0x5548);

        /* Create an instance of UDB using the config file */
        UDB* udb = udb_create_instance(udb_config);
        if (udb == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(0);
        }

        /* Get a connection to the UDB */
        UDB_CONNECTION* udb_conn = udb_create_connection(udb);
        if (udb_conn == nullptr)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(0);
        }

        /* ping the connection */
        if (udb_ping(udb_conn) != UDB_RESULT_SUCCESS)
        {
            std::cout << "error_occured: " << get_error_message();
            exit(0);
        }

    /* some random data */
        char test_data_1[] = "The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs. "
                           "How quickly daft jumping zebras vex. Crazy Fredrick bought many very exquisite opal jewels. "
                           "The five boxing wizards jump quickly. Mr. Jock, TV quiz PhD, bags few lynx. Sphinx of black "
                           "quartz, judge my vow. Jackdaws love my big sphinx of quartz. Bright vixens jump; dozy fowl "
                           "quack. Sphinx of black quartz, judge my vow. Waltz, nymph, for quick jigs vex Bud. How vexingly "
                           "quick daft zebras jump! Quick zephyrs blow, vexing daft Jim. Cozy lummox gives smart squid "
                           "who asks for job pen. A wizard’s job is to vex chumps quickly in fog. The quick brown fox "
                           "jumps over the lazy dog.";

        char test_data_2[] = "Jumbled quirky text with no apparent meaning. Zxvqwyj fmgpa uosm xad no edlkouo yzbrmyi. "
                             "Pqnvwq zlxud xnbq tlz btvi gjpdqlbf asikqls qypfz mntuo. Hkqnyrd mdwfh pmqg eaua fgyvvb "
                             "qzqweufi dtdqm gnbge. Icnvw cmhbq yvqf tkw dmtgs wtrgojdqz hggcqde apmdr. Kojzptz qbtsi "
                             "mznq bvwu uybwcnc wulvzkki gppdm jqmn. Sobjp hfhaq kkih glvyzlj ewztnq yilxrswe nsqwtn "
                             "epqq. Vtxh dgrlq byng itze adcsrogy zrizxsq ydnayb vtay. Vqthd jkbqx ksgo ykmvzz fljo "
                             "kgnxfuab pdmgcw xsifx.";

        char test_data_3[] = "1G3#%&6d2$!a9B7@4C5c8D0e#F!hIjKgMfLhNjPiQkOhRl#SpUtVrXqYsZtWu+v=x-wy/z";

        char test_key[] = "This_is_a_user_defined_key";
        char test_label_1[] = "red";
        char test_label_2[] = "blue";
        char* test_labels[] = {test_label_1, test_label_2};

    /* for adding single document into the database */
        UDB_DATA key(test_key, strlen(test_key));
        UDB_DATA value(test_data_1, strlen(test_data_1));

        UDB_DOCUMENT* test_doc_1 = udb_init_document(&key, &value, test_labels, 2);

        /* create write query */
        UDB_WRITE_QUERY* test_write_query = udb_create_write_query();
        udb_write_query_add_document(test_write_query, test_doc_1);

        if (udb_add(udb_conn, test_write_query) != UDB_RESULT_SUCCESS)
        {
            std::cout << "error: " << get_error_message();
            exit(0);
        }

    /* cleanup */
        udb_destroy_write_query(&test_write_query);
        udb_destroy_connection(udb_conn);
        udb_destroy_instance(udb);
        udb_destroy_config(udb_config);

return 0;
}
