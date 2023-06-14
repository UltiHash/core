//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_license.h"
#include "util/exception.h"
#include "logging/logging_boost.h"
#include "io/temp_file.h"
#include "io/sha512.h"

#include "LicenseSpring/Exceptions.h"

#include <fstream>
#include <string>
#include <iostream>
#include <utility>

#include "boost/process.hpp"
#include "boost/asio.hpp"

namespace uh::licensing {

    // ---------------------------------------------------------------------

    check_license::check_license(std::filesystem::path license_path, check_license::license_type license_type,
                                 std::string apiKey_encrypted, std::string sharedKey_encrypted, std::string productId_enrypted, std::string appName,
                                 std::string appVersion) :
            license_path(std::move(license_path)), licenseTypeInternal(license_type),
            appName(std::move(appName)), appVersion(std::move(appVersion)),
            apiKey_crypt(std::move(apiKey_encrypted)), sharedKey_crypt(std::move(sharedKey_encrypted)), productId_crypt(std::move(productId_enrypted))
            {
        if(this->appName.empty())
            this->appName = check_app_name();

        if(this->appVersion.empty())
            this->appVersion = check_app_version();
    }

    // ---------------------------------------------------------------------

    check_license::role check_license::check_role()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(role_string)) {
                line = line.substr(role_string.size(), line.size());

                if (line == "agency_node")
                    return check_license::role::AGENCY_NODE;

                if (line == "data_node")
                    return check_license::role::DATA_NODE;
            }
        }

        return check_license::role::THROW_ROLE;
    }

    // ---------------------------------------------------------------------

    check_license::license_type check_license::check_license_type()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(license_type_string)) {
                line = line.substr(license_type_string.size(), line.size());

                if (line == airgap_license_string)
                    return check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION;

                if (line == floating_license_string)
                    return check_license::license_type::FLOATING_ONLINE_USER_LICENSE;
            }
        }

        return check_license::license_type::THROW_LICENSE_TYPE;
    }

    // ---------------------------------------------------------------------

    std::string check_license::check_app_name()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(appName_string)) {
                line = line.substr(appName_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

    std::string check_license::check_app_version()
    {
        if(!std::filesystem::exists(license_path) || std::filesystem::is_directory(license_path))
            return {};

        std::fstream license_file_stream(license_path, std::ios_base::in);

        for (std::string line; std::getline(license_file_stream, line);) {
            if (line.starts_with(appVersion_string)) {
                line = line.substr(appVersion_string.size(), line.size());
                return line;
            }
        }

        return {};
    }

    // ---------------------------------------------------------------------

    bool check_license::valid() {
        return true;
    }

    // ---------------------------------------------------------------------

    io::file check_license::write_license_file(check_license::role licenseRole, const std::string &app_name_input,
                                               const std::string &app_version_input)
            {
        std::string role_set_string, license_type_set_string;

        switch (licenseRole) {
            case role::AGENCY_NODE:
                role_set_string = "agency_node";
                break;
            case role::DATA_NODE:
                role_set_string = "data_node";
                break;
            default:
                THROW(util::exception, "No license role detected!");
        }

        switch (licenseTypeInternal) {
            case license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION:
                license_type_set_string = airgap_license_string;
                break;
            case license_type::FLOATING_ONLINE_USER_LICENSE:
                license_type_set_string = floating_license_string;
                break;
            default:
                THROW(util::exception, "No license type detected!");
        }

        std::filesystem::path out_license_path = license_path / (std::string(role_set_string) + ".lic");

        if (std::filesystem::exists(out_license_path))
            THROW(util::exception, "A license already existed on path \"" + license_path.string() + "\" !");

        {
            std::filesystem::path write_tmp_path = license_path;
            if(write_tmp_path.extension() == ".lic")
                write_tmp_path = write_tmp_path.parent_path();
            io::temp_file write_temp(write_tmp_path);

            write_temp.write(std::string(appName_string) + app_name_input + "\n");
            write_temp.write(std::string(appVersion_string) + app_version_input + "\n");
            write_temp.write(std::string(role_string) + role_set_string + "\n");
            write_temp.write(std::string(license_type_string) + license_type_set_string + "\n");

            if(std::filesystem::is_directory(license_path)){
                license_path = license_path / (role_set_string + ".lic");
                write_temp.release_to(license_path);
            }

            else {
                if(license_path.extension() != ".lic")
                    license_path += ".lic";
                write_temp.release_to(license_path);
            }
        }

        io::file out_file(license_path,std::ios_base::app);

        return out_file;
    }

    // ---------------------------------------------------------------------

    bool check_license::is_written() {
        return std::filesystem::exists(license_path) and std::filesystem::is_regular_file(license_path);
    }

    // ---------------------------------------------------------------------

    const std::filesystem::path &check_license::getLicensePath() const {
        return license_path;
    }

    // ---------------------------------------------------------------------

    int check_license::licenseRegister(const std::shared_ptr<LicenseSpring::LicenseManager> &licenseManager,
                                       const LicenseSpring::LicenseID &licenseId) {

        LicenseSpring::License::ptr_t license = licenseManager->reloadLicense();
        if(!license_check(license)){
            try {
                license = licenseManager->activateLicense(licenseId);
            }
            catch (LicenseSpring::ProductNotFoundException) {
                ERROR << "Product not found on server, please check your product configuration." << std::endl;
                std::cout << "Product not found on server, please check your product configuration." << std::endl;
                return 0;
            }
            catch (LicenseSpring::LicenseNotFoundException) {
                ERROR << "License could not be found on server." << std::endl;
                std::cout << "License could not be found on server." << std::endl;
                return 0;
            }
            catch (LicenseSpring::LicenseStateException) {
                ERROR << "License is currently expired or disabled." << std::endl;
                std::cout << "License is currently expired or disabled." << std::endl;
                return 0;
            }
            catch (LicenseSpring::LicenseNoAvailableActivationsException) {
                ERROR << "No available activations remaining on license." << std::endl;
                std::cout << "No available activations remaining on license." << std::endl;
                return 0;
            }
            catch (LicenseSpring::CannotBeActivatedNowException) {
                ERROR << "Current date is not at start date of license." << std::endl;
                std::cout << "Current date is not at start date of license." << std::endl;
                return 0;
            }
            catch (LicenseSpring::SignatureMismatchException) {
                ERROR << "Signature on LicenseSpring server is invalid." << std::endl;
                std::cout << "Signature on LicenseSpring server is invalid." << std::endl;
                return 0;
            }
            catch (...) {
                ERROR << "Possible connection issue." << std::endl;
                std::cout << "Possible connection issue." << std::endl;
                return 0;
            }
        }

        if (license != nullptr) {
            license->check();
        }
        //We can then extract our custom fields as a vector of CustomField objects as so:
        std::vector<LicenseSpring::CustomField> custom_vec = license->customFields();

        boost::asio::io_service ios;
        std::string lshw_result;

        boost::process::async_pipe ap(ios);

        boost::process::child c( "usr/bin/bash", "lshw -class bus -sanitize", boost::process::std_out > ap);

        boost::asio::async_read(ap, boost::asio::buffer(lshw_result),
                                [](const boost::system::error_code &ec, std::size_t size){});

        ios.run();
        c.wait();
        int result = c.exit_code();

        if(result)
            THROW(util::exception,"Could not read lshw motherboard identity!");

        if (custom_vec.empty()) {

            license->addDeviceVariable("DeviceIdentityHash", lshw_result);

            std::vector<LicenseSpring::DeviceVariable> device_vec;

            try {
                license->sendDeviceVariables();
                device_vec = license->getDeviceVariables(true);
                return device_vec[0].name() == "DeviceIdentityHash" and device_vec[0].value() == lshw_result;
            }
            catch (...) {
                std::cout << "Most likely a network connection issue, please check your connection." << std::endl;
                return false;
            }

        }
        else{
            //check if DeviceIdentityHash matches
            return custom_vec[0].fieldName() == "DeviceIdentityHash" and custom_vec[0].fieldValue() == "dummyident";
        }
    }

    bool check_license::license_check(const LicenseSpring::License::ptr_t& license) {
        //First we'll run a online check. This will check your license on the
        //LicenseSpring servers, and sync up your local license to match your online
        if ( license != nullptr )
        {
            try
            {
                std::cout << "Checking license online..." << std::endl;
                license->check();
                std::cout << "License successfully checked" << std::endl;
            }
            catch ( LicenseSpring::LicenseStateException )
            {
                WARNING << "Online license is not valid" << std::endl;
                std::cout << "Online license is not valid" << std::endl;
                if ( !license->isActive() ){
                    std::cout << "License is inactive" << std::endl;
                    return false;
                }
                if ( license->isExpired() ){
                    std::cout << "License is expired" << std::endl;
                    return false;
                }
                if ( !license->isEnabled() ){
                    std::cout << "License is disabled" << std::endl;
                    return false;
                }
            }

            //We use localCheck() in LicenseSpring/License.h in the include folder.This is
            //useful to check if the license hasn't been copied over from another device, and
            //that the license is still valid. Note: valid and active are not the same thing. See
            //tutorial [link here] to learn the difference.
            try
            {
                std::cout << "License successfully loaded, performing local check of the license..." << std::endl;
                license->localCheck();
                std::cout << "Local validation successful" << std::endl;
            }
            catch (LicenseSpring::LocalLicenseException) { //Exception if we cannot read the local license or the local license file is corrupt
                ERROR << "Could not read previous local license. Local license may be corrupt. "
                      << "Create a new local license by activating your license." << std::endl;

                std::cout << "Could not read previous local license. Local license may be corrupt. "
                          << "Create a new local license by activating your license." << std::endl;
                return false;
            }
            catch (LicenseSpring::LicenseStateException) {
                ERROR << "Current license is not valid" << std::endl;
                std::cout << "Current license is not valid" << std::endl;
                if (!license->isActive()) {
                    ERROR << "License is inactive" << std::endl;
                    std::cout << "License is inactive" << std::endl;
                    return false;
                }
                if (license->isExpired()) {
                    ERROR << "License is expired" << std::endl;
                    std::cout << "License is expired" << std::endl;
                    return false;
                }
                if (!license->isEnabled()) {
                    ERROR << "License is disabled" << std::endl;
                    std::cout << "License is disabled" << std::endl;
                    return false;
                }
            }
            catch (LicenseSpring::ProductMismatchException) {
                ERROR << "License does not belong to configured product." << std::endl;
                std::cout << "License does not belong to configured product." << std::endl;
                return false;
            }
            catch (LicenseSpring::DeviceNotLicensedException) {
                ERROR << "License does not belong to current computer." << std::endl;
                std::cout << "License does not belong to current computer." << std::endl;
                return false;
            }
            catch (LicenseSpring::VMIsNotAllowedException) {
                ERROR << "Currently running on VM, when VM is not allowed." << std::endl;
                std::cout << "Currently running on VM, when VM is not allowed." << std::endl;
                return false;
            }
            catch (LicenseSpring::ClockTamperedException) {
                ERROR << "Detected cheating with system clock." << std::endl;
                std::cout << "Detected cheating with system clock." << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "No local license found";
            return false;
        }
        return true;
    }

} // namespace uh::licensing