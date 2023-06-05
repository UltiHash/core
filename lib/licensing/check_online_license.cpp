//
// Created by benjamin-elias on 01.06.23.
//

#include "check_online_license.h"

namespace uh::licensing{

    // ---------------------------------------------------------------------

    check_online_license::check_online_license(std::filesystem::path license_folder) :
            check_license(std::move(license_folder)){}

    // ---------------------------------------------------------------------

    check_license::role check_online_license::check_role() {
        //TODO: implement online license role
        return check_license::role::THROW_ROLE;
    }

    // ---------------------------------------------------------------------

    bool check_online_license::valid() {
        //TODO: online implementation if license is valid and exists
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing
