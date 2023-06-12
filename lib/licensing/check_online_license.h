//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_ONLINE_LICENSE_H
#define CORE_CHECK_ONLINE_LICENSE_H

#include "licensing/check_license.h"

#include <utility>

namespace uh::licensing {

    class check_online_license : public check_license {

    public:

        explicit check_online_license(const std::filesystem::path &license_file, std::string apiKey,
                                      std::string sharedKey,
                                      std::string productId, std::string appName = "", std::string appVersion = "",
                                      std::string userName = "", std::string password = "");

        bool valid() override;

        void write_license(check_license::role licenseRole, const std::string &app_name_input,
                           const std::string &app_version_input,
                           const std::string &username_input, const std::string &password_input);

        std::string check_user_name();

        std::string check_password();

    private:
        std::string userName{}, password{};
        const std::string_view user_name_string = "user_name: ";
        const std::string_view password_string = "password: ";
    };

} // namespace uh::licensing

#endif //CORE_CHECK_ONLINE_LICENSE_H
