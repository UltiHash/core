//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_LICENSING_OPTIONS_H
#define CORE_LICENSING_OPTIONS_H

#include "options/options.h"
#include <filesystem>
#include <utility>

namespace uh::options {

// ---------------------------------------------------------------------

struct licensing_config
{
public:
    std::string licensing_path;
    std::string license_key;
    std::string license_user;
    std::string license_password;

    licensing_config() = default;

    licensing_config(std::string path, std::string key){
        licensing_path = std::move(path);
        license_key = std::move(key);
    }

    licensing_config(std::string path, std::string user, std::string pass){
        licensing_path = std::move(path);
        license_user = std::move(user);
        license_password = std::move(pass);
    }

    [[nodiscard]] bool unchanged() const{
        return licensing_path.empty() and
        license_key.empty()
        and license_user.empty()
        and license_password.empty();
    }
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
