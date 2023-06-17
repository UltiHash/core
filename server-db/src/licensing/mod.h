//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_MOD_H
#define CORE_MOD_H

#include <licensing/license_package.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>
#include "options/licensing_options.h"

namespace uh::dbn::licensing {

    // ---------------------------------------------------------------------

    enum class LicenseTypeEnum {
        AirgapOnlineActivationLicense, OtherLicense
    };

    static std::unordered_map<std::string, LicenseTypeEnum> string2licensetype = {
            {"AirgapOnline", LicenseTypeEnum::AirgapOnlineActivationLicense},
            {"OtherLicense", LicenseTypeEnum::OtherLicense}
    };

    // ---------------------------------------------------------------------

    class mod {
    public:
        explicit mod(const uh::options::licensing_config &cfg);

        ~mod();

        void start();

        uh::licensing::license_package &license_package();

    private:
        struct impl;
        std::unique_ptr<impl> m_impl;
    };

    // ---------------------------------------------------------------------

} // namespace uh::dbn::licensing

namespace uh::dbn {

    // ---------------------------------------------------------------------

    uh::dbn::licensing::mod *licensing_global_module;

    // ---------------------------------------------------------------------

} // namespace uh::dbn

#endif //CORE_MOD_H
