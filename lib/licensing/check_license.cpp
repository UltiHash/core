//
// Created by benjamin-elias on 06.06.23.
//

#include "licensing/check_license.h"
#include "util/exception.h"
#include "logging/logging_boost.h"

#include <fstream>
#include <string>
#include <iostream>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_license::role check_license::check_role()
    {
        std::fstream license_file_stream(license_path,std::ios_base::in);

        for(std::string line; std::getline(license_file_stream,line);)
        {
            if(line.starts_with(role_string))
            {
                line = line.substr(role_string.size(),line.size());

                if(line == "agency_node")
                    return check_license::role::AGENCY_NODE;

                if(line == "data_node")
                    return check_license::role::DATA_NODE;
            }
        }

        return check_license::role::THROW_ROLE;
    }

    // ---------------------------------------------------------------------

    check_license::license_type check_license::check_license_type()
    {
        std::fstream license_file_stream(license_path,std::ios_base::in);

        for(std::string line; std::getline(license_file_stream,line);)
        {
            if(line.starts_with(license_type_string))
            {
                line = line.substr(license_type_string.size(),line.size());

                if(line == "airgap_license_with_online_activation")
                    return check_license::license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION;

                if(line == "floating_user_license")
                    return check_license::license_type::FLOATING_ONLINE_USER_LICENSE;
            }
        }

        return check_license::license_type::THROW_LICENSE_TYPE;
    }

    bool check_license::valid() {
        return true;
    }

    void check_license::write_license_base(check_license::role licenseRole, check_license::license_type licenseType)
    {
        std::string role_set_string, license_type_set_string;

        switch (licenseRole)
        {
            case role::AGENCY_NODE:
                role_set_string = "agency_node";
                break;
            case role::DATA_NODE:
                role_set_string = "data_node";
                break;
            default:
                THROW(util::exception,"No license role detected!");
        }

        switch (licenseType)
        {
            case license_type::AIRGAP_LICENSE_WITH_ONLINE_ACTIVATION:
                license_type_set_string = "airgap_license_with_online_activation";
                break;
            case license_type::FLOATING_ONLINE_USER_LICENSE:
                license_type_set_string = "floating_online_user_license";
                break;
            default:
                THROW(util::exception,"No license type detected!");
        }

        std::filesystem::path out_license_path = license_path / (role_string + ".lic");

        if(std::filesystem::exists(out_license_path))
            THROW(util::exception,"A license already existed on path \""+license_path.string()+"\" !");

        std::fstream license_file_stream(license_path,std::ios_base::out);

        license_file_stream << role_string << role_set_string << std::endl;
        license_file_stream << license_type_string << license_type_set_string << std::endl;
    }

    // ---------------------------------------------------------------------

    int check_license::licenseRegister(const std::shared_ptr<LicenseSpring::LicenseManager> &licenseManager,
                                       const LicenseSpring::LicenseID& licenseId) {
        //reloadLicense() will return a pointer to the local license stored
        //on the end-user's device if they have one that matches the current
        //configuration i.e. API key, Shared key, and product code.
        LicenseSpring::License::ptr_t license = nullptr;

        try
        {
            license = licenseManager->reloadLicense();
            if ( license != nullptr )
            {
                license->localCheck(); //always good to do a local check whenever you run your program
            }
        }
        catch ( LicenseSpring::LocalLicenseException )
        { //Exception if we cannot read the local license or the local license file is corrupt
            ERROR << "Could not read previous local license. Local license may be corrupt. "
                      << "Create a new local license by activating your license." << std::endl;

            std::cout << "Could not read previous local license. Local license may be corrupt. "
                      << "Create a new local license by activating your license." << std::endl;
            return 0;
        }
        catch ( LicenseSpring::LicenseStateException )
        {
            ERROR << "Current license is not valid" << std::endl;
            std::cout  << "Current license is not valid" << std::endl;
            if ( !license->isActive() )
            {
                ERROR << "License is inactive" << std::endl;
                std::cout << "License is inactive" << std::endl;
            }
            if ( license->isExpired() )
            {
                ERROR << "License is expired" << std::endl;
                std::cout << "License is expired" << std::endl;
            }
            if ( !license->isEnabled() )
            {
                ERROR << "License is disabled" << std::endl;
                std::cout << "License is disabled" << std::endl;
            }
        }
        catch ( LicenseSpring::ProductMismatchException )
        {
            ERROR << "License does not belong to configured product." << std::endl;
            std::cout << "License does not belong to configured product." << std::endl;
        }
        catch ( LicenseSpring::DeviceNotLicensedException )
        {
            ERROR << "License does not belong to current computer." << std::endl;
            std::cout << "License does not belong to current computer." << std::endl;
        }
        catch ( LicenseSpring::VMIsNotAllowedException )
        {
            ERROR << "Currently running on VM, when VM is not allowed." << std::endl;
            std::cout << "Currently running on VM, when VM is not allowed." << std::endl;
        }
        catch ( LicenseSpring::ClockTamperedException )
        {
            ERROR << "Detected cheating with system clock." << std::endl;
            std::cout << "Detected cheating with system clock." << std::endl;
        }

        //Here we'll make sure our license is activated before we start testing with
        //custom fields and device variables.
        try
        {
            license = licenseManager->activateLicense( licenseId );

        }
        catch ( LicenseSpring::ProductNotFoundException )
        {
            ERROR << "Product not found on server, please check your product configuration." << std::endl;
            std::cout << "Product not found on server, please check your product configuration." << std::endl;
            return 0;
        }
        catch ( LicenseSpring::LicenseNotFoundException )
        {
            ERROR << "License could not be found on server." << std::endl;
            std::cout << "License could not be found on server." << std::endl;
            return 0;
        }
        catch ( LicenseSpring::LicenseStateException )
        {
            ERROR << "License is currently expired or disabled." << std::endl;
            std::cout << "License is currently expired or disabled." << std::endl;
            return 0;
        }
        catch ( LicenseSpring::LicenseNoAvailableActivationsException )
        {
            ERROR << "No available activations remaining on license." << std::endl;
            std::cout << "No available activations remaining on license." << std::endl;
            return 0;
        }
        catch ( LicenseSpring::CannotBeActivatedNowException )
        {
            ERROR << "Current date is not at start date of license." << std::endl;
            std::cout << "Current date is not at start date of license." << std::endl;
            return 0;
        }
        catch ( LicenseSpring::SignatureMismatchException )
        {
            ERROR << "Signature on LicenseSpring server is invalid." << std::endl;
            std::cout << "Signature on LicenseSpring server is invalid." << std::endl;
            return 0;
        }
        catch ( ... )
        {
            ERROR << "Possible connection issue." << std::endl;
            std::cout << "Possible connection issue." << std::endl;
            return 0;
        }


        //Once our local license is synced up/updated with the online license, we should have the most up-to-date
        //custom fields. There are two ways to update our custom fields to be in sync with our online product/license
        //1. Activating a license will update all our custom fields on our local license.
        //2. Running an online check (as long as it doesn't throw an exception), will also sync up our custom fields.
        //on our local license.
        if( license != nullptr )
        {
            license->check();
        }
        //We can then extract our custom fields as a vector of CustomField objects as so:
        std::vector<LicenseSpring::CustomField> custom_vec = license->customFields();

        //Note: it is possible for our custom fields to be empty if we haven't added anything so be aware.
        for ( LicenseSpring::CustomField custom_field : custom_vec )
        {
            //Here we display our key:value pairs.
            std::cout << "Key: " << custom_field.fieldName() << " |Value: " << custom_field.fieldValue() << std::endl;

            //You can also update the custom field key:value pairs as well, however, this
            //will only affect the custom fields locally on your device, and will not in any way change
            //the custom field key:value pairs set up on the LicenseSpring platform. The only way to change
            //the custom field values on the LicenseSpring platoform, is manually, and not with code.
            custom_field.setFieldName( "New " + custom_field.fieldName() );
            custom_field.setFieldValue( "New " + custom_field.fieldValue() );
            std::cout << "New Key: " << custom_field.fieldName() << "|New Value: " << custom_field.fieldValue() << std::endl;
        }



        //Since device variables are the opposite of custom fields, we create them on the user end then sync with the backend
        std::cout << "Enter 'y' to create a device variable, any other input to skip." << std::endl;
        std::string response;
        std::getline( std::cin, response );

        //Through taking in the name and value of the device variable we are able to pass them into addDeviceVariable() to create the device
        //variable locally. The platform still does not contain device variables after addDeviceVariable()

        //To update device variables, use the same varName as the variable you want to update, and use your
        //updated value for varValue.
        if ( response == "y" )
        {

            std::cout << "Name of Variable: ";
            std::string varName;
            std::getline( std::cin, varName );

            std::cout << "Value of Variable: ";
            std::string varValue;
            std::getline( std::cin, varValue );
            license->addDeviceVariable( varName, varValue );
            //Note, you can also create a DeviceVariable object and pass that in as a parameter instead.
            //Furthermore, there is an optional third parameter, that by default is set true. When true,
            //it'll save the DeviceVariable on your local license. When false, it will not add the Device
            //Variable to your local license.
        }

        //sendDeviceVariables() sends the device variables that have been created and stored locally to the backend, which syncs up
        //both ends to have matching device variables. Creating a vector of device variables and setting it to equal license->getDeviceVariables()
        //retrieves all the device variables currently stored locally. The true that we pass into getDeviceVariables() makes the function check
        //the backend, which contains all the device variables since we just sent local changes to the backend right before getting them.

        std::vector<LicenseSpring::DeviceVariable> device_vec;
        try
        {
            license->sendDeviceVariables();
            device_vec = license->getDeviceVariables( true );
        }
        catch ( ... )
        {
            std::cout << "Most likely a network connection issue, please check your connection." << std::endl;
        }

        //Looping through each device variable, we are able to print the names and values of each device variable using name() and value()
        //respectively.

        for ( const LicenseSpring::DeviceVariable& device_variable : device_vec )
        {
            std::cout << "Device Variable Name: " << device_variable.name() << " |Value: " << device_variable.value() << std::endl;
        }

        return 0;
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing