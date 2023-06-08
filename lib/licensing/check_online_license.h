//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_ONLINE_LICENSE_H
#define CORE_CHECK_ONLINE_LICENSE_H

#include "licensing/check_license.h"

#include <utility>

namespace uh::licensing{

    class check_online_license: public check_license {

    public:

        explicit check_online_license(std::filesystem::path license_folder, std::string appName, std::string appVersion,
                                      std::string userName, std::string password);

        bool valid() override;

    private:
        std::string userName, password;
    };

} // namespace uh::licensing

#endif //CORE_CHECK_ONLINE_LICENSE_H
