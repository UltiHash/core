//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <filesystem>
#include <utility>

namespace uh::licensing{

    class check_license{

    public:

        /**
         *
         * Check if `role` is enabled in the configured license.
         * @throw if `role`does not match given license
         */
        enum class role: unsigned char{
            THROW_ROLE,
            AGENCY_NODE,
            DATA_NODE
        };

        /**
         * remembers the license path to be checked on request
         *
         * @param license_folder is the folder where a license file is stored
         */
        explicit check_license(std::filesystem::path license_folder): license_path(std::move(license_folder)){};

        /**
         *
         * @return role that was initialized by license file
         */
        virtual role check_role() = 0;

        /**
         * the default license is not timed
         *
         * @param license_path is the path to the license file
         * @return if the license file is valid for the implemented service role and features
         */
        virtual bool valid() = 0;

    protected:
        std::filesystem::path license_path;
    };

} // namespace uh::licensing

#endif //CORE_CHECK_LICENSE_H
