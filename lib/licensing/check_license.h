//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_LICENSE_H
#define CORE_CHECK_LICENSE_H

#include <filesystem>

namespace uh::licensing{

    class check_license{

        /**
         *
         * @param license_path is the path to the license file
         * @return if the license file is valid for the implemented service role and features
         */
        virtual bool valid() = 0;

    };

}

#endif //CORE_CHECK_LICENSE_H
