//
// Created by juan on 12.12.22.
//

#ifndef DB_NODE_DB_CONFIG_H
#define DB_NODE_DB_CONFIG_H

#include <filesystem>

namespace uh::dbn {

// ---------------------------------------------------------------------

    struct db_config {
        constexpr static std::string_view DEFAULT_DB_ROOT = "./UH_DB_ROOT";
        std::filesystem::path db_root = std::string(DEFAULT_DB_ROOT);
        bool create_new_root = false;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn
#endif //DB_NODE_DB_CONFIG_H
