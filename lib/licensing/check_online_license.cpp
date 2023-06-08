//
// Created by benjamin-elias on 01.06.23.
//

#include "check_online_license.h"

#include <LicenseSpring/Configuration.h>
#include <LicenseSpring/EncryptString.h>
#include <LicenseSpring/LicenseManager.h>
#include <LicenseSpring/Exceptions.h>

#include <iostream>
#include <thread>

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_online_license::check_online_license(std::filesystem::path license_folder) :
            check_license(std::move(license_folder)){}


    // ---------------------------------------------------------------------

    bool check_online_license::valid() {
        //TODO: online implementation if license is valid and exists

    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing
