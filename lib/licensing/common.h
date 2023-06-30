//
// Created by max on 28.06.23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <string>
#include <filesystem>
#include <licensing_config.h>

namespace uh::licensing {

// ---------------------------------------------------------------------

    struct license_config {
        std::string productId;
        std::string appName;
        std::string appVersion;
        std::filesystem::path path;
    };

// ---------------------------------------------------------------------

}
#endif //CORE_COMMON_H
