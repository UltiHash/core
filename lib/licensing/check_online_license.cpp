//
// Created by benjamin-elias on 01.06.23.
//

#include "check_online_license.h"

#include <utility>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_online_license::check_online_license(const std::filesystem::path &license_file, std::string apiKey_encrypted,
                                               std::string sharedKey_encrypted,
                                               std::string productId_encrypted, std::string appName, std::string appVersion,
                                               std::string userName, std::string password) :
            check_license(license_file,
                          license_type::FLOATING_ONLINE_USER_LICENSE,
                          std::move(apiKey_encrypted),
                          std::move(sharedKey_encrypted),
                          std::move(productId_encrypted), std::move(appName),
                          std::move(appVersion)),
                          userName(std::move(userName)), password(std::move(password))
                          {
        if(this->userName.empty())
          this->userName = check_user_name();

        if(this->password.empty())
          this->password = check_password();
    }

    // ---------------------------------------------------------------------

    bool check_online_license::valid()
    {
        return userName == check_user_name() and password == check_password();
    }

    // ---------------------------------------------------------------------

    void check_online_license::write_license(check_license::role licenseRole, const std::string &app_name_input,
                                             const std::string &app_version_input,
                                             const std::string &username_input, const std::string &password_input)
                                             {
        auto out_file = write_license_file(licenseRole, app_name_input, app_version_input);
        out_file.write(std::string(user_name_string) + username_input + "\n");
        out_file.write(std::string(password_string) + password_input + "\n");
    }

    // ---------------------------------------------------------------------

    std::string check_online_license::check_user_name()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(user_name_string)) {
                line = line.substr(user_name_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

    std::string check_online_license::check_password()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(password_string)) {
                line = line.substr(password_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing
