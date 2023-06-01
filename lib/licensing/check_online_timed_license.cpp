//
// Created by benjamin-elias on 01.06.23.
//

#include "check_online_timed_license.h"

namespace uh::licensing {

    // ---------------------------------------------------------------------

    bool check_online_timed_license::valid() {
        return check_online_license::valid() and valid_timing();
    }

    // ---------------------------------------------------------------------

    bool check_online_timed_license::valid_timing() {
        return false;
    }

    // ---------------------------------------------------------------------

} // namespace uh::licensing