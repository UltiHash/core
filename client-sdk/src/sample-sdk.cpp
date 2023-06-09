#include <iostream>
#include "../include/ultidb.h"

int main()
{
     std::cout << "UDB version " << get_sdk_version() << "\n";

     /* Get a pointer to the UDB Config File */
    UDB_CONFIG* udb_config = udb_create_config();
    if (udb_config == nullptr)
    {
        std::cout << "error_occured: " << get_error_message();
        exit(0);
    }
    udb_config_set_agencynode(udb_config, "localhost", 0x5548);

    /* Create an instance of UDB using the config file */
    UDB* udb = udb_create_instance(udb_config);
    if (udb == nullptr)
    {
        std::cout << "error_occured: " << get_error_message();
        exit(0);
    }

    /* integrate data */
    char client_data[] ="Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut"
                        " labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco "
                        "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
                        "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat"
                        " non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    char hash_received[65] = {0};
    auto result_code = udb_integrate(udb, hash_received, sizeof(hash_received), client_data, sizeof(client_data));
    if ( result_code != UDB_RESULT_SUCCESS)
    {
        std::cout << "error_occured: " << get_error_message();
        exit(0);
    }

    /* retrieve data */
    std::cout << hash_received << '\n';

    char data_container[2000] = {0};
    result_code = udb_retrieve(udb, data_container, sizeof(data_container), hash_received);
    if (result_code != UDB_RESULT_SUCCESS)
    {
        std::cout << "error occured: " << get_error_message();
    }
    std::cout << data_container << '\n';

    /* cleanup */
    udb_destroy_instance(udb);
    udb_destroy_config(udb_config);

    return 0;
}
