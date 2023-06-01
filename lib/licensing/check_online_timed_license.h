//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_ONLINE_TIMED_LICENSE_H
#define CORE_CHECK_ONLINE_TIMED_LICENSE_H

#include "licensing/check_online_license.h"
#include "licensing/check_timed.h"

namespace uh::licensing {

    class check_online_timed_license: public check_online_license, public check_timed{

    public:

        bool valid() override;

    private:
        bool valid_timing() override;
    };

} // licensing

#endif //CORE_CHECK_ONLINE_TIMED_LICENSE_H
