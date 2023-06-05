//
// Created by benjamin-elias on 01.06.23.
//

#include "check_airgap_license.h"

namespace uh::licensing {

    // ---------------------------------------------------------------------

    check_airgap_license::check_airgap_license(std::filesystem::path license_folder):
            check_license(std::move(license_folder)){}

    // ---------------------------------------------------------------------

    check_license::role check_airgap_license::check_role() {
        //TODO: implement check role for air-gap license
        return check_license::role::THROW_ROLE;
    }

    // ---------------------------------------------------------------------

    bool check_airgap_license::valid() {
        //TODO: local implementation if license is valid and exists
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing