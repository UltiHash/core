//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_LICENSING_OPTIONS_H
#define CORE_LICENSING_OPTIONS_H

#include "options/options.h"
#include <filesystem>

namespace uh::options {

    // ---------------------------------------------------------------------

    struct licensing_config
    {
        std::string licensing_path;
        std::string license_key;
        std::string license_user;
        std::string license_password;
        std::string license_type;
    };

// ---------------------------------------------------------------------

    class licensing_options : public uh::options::options
    {
    public:
        licensing_options();

        [[nodiscard]] const licensing_config& config() const;

    protected:
        licensing_config m_config;

    };

// ---------------------------------------------------------------------

} // namespace uh::options

#endif //CORE_LICENSING_OPTIONS_H
