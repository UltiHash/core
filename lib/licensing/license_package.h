//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_LICENSE_PACKAGE_H
#define CORE_LICENSE_PACKAGE_H

#include "licensing/check_license.h"
#include "licensing/licensed_feature.h"

#include <vector>

namespace uh::licensing {

    enum feature_list{
        METRICS,
        TERRABYTE100LIMIT
    };

    //TODO: template of license type
    class license_package {

    public:


    private:
        std::vector<licensed_feature> features;
    };

} // namespace uh::licensing

#endif //CORE_LICENSE_PACKAGE_H
