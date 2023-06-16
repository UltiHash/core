//
// Created by benjamin-elias on 15.06.23.
//

#ifndef CORE_MOD_H
#define CORE_MOD_H

#include <protocol/client_pool.h>
#include <metrics/storage_metrics.h>
#include <persistence/storage/scheduled_compressions_persistence.h>
#include <licensing/license_package.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>

namespace uh::dbn::licensing {

    // ---------------------------------------------------------------------

    enum class LicenseTypeEnum {AirgapOnlineActivationLicense, OtherLicense};

    static std::unordered_map<std::string, LicenseTypeEnum> string2backendtype = {
            {"AirgapOnline", LicenseTypeEnum::AirgapOnlineActivationLicense},
            {"OtherLicense", LicenseTypeEnum::OtherLicense}
    };

// ---------------------------------------------------------------------

    struct licensing_config
    {
        constexpr static std::string_view default_license_root = "./DEFAULT_LICENSE_ROOT";
        constexpr static std::string_view default_license_type = "AirgapOnline";
        std::filesystem::path license_root = std::string(default_license_root);
        std::string license_type = std::string(default_license_type);
        bool create_new_license = false;
    };

// ---------------------------------------------------------------------

    class mod
    {
    public:
        explicit mod(const licensing_config& cfg);
        ~mod();

        void start();

        uh::licensing::license_package& license_package();

    private:
        struct impl;
        std::unique_ptr<impl> m_impl;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn::licensing

#endif //CORE_MOD_H
