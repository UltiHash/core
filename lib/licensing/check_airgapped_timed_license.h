//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_AIRGAPPED_TIMED_LICENSE_H
#define CORE_CHECK_AIRGAPPED_TIMED_LICENSE_H

#include "licensing/check_airgap_license.h"
#include "licensing/check_timed.h"

namespace uh::licensing {

    class check_airgapped_timed_license: public check_airgap_license, public check_timed{

    public:

        bool valid() override;

    private:
        bool valid_timing() override;
    };

} // namespace uh::licensing

#endif //CORE_CHECK_AIRGAPPED_TIMED_LICENSE_H
