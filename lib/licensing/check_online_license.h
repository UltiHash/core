//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_ONLINE_LICENSE_H
#define CORE_CHECK_ONLINE_LICENSE_H

#include "licensing/check_license.h"

namespace uh::licensing{

    class check_online_license: public check_license {

        bool valid() override;

    };

} // namespace uh::licensing

#endif //CORE_CHECK_ONLINE_LICENSE_H
