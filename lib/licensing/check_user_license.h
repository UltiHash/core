//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_ONLINE_LICENSE_H
#define CORE_CHECK_ONLINE_LICENSE_H

#include "licensing/check_license.h"

#include <utility>

namespace uh::licensing
{

class check_user_license: public check_license
{

public:

    /*
     * license model of online activation and floating use with a time limit set on license cloud
     */
    explicit check_user_license(uh::licensing::license_config license_config,
                                uh::licensing::api_config apiKey_input,
                                uh::licensing::credential_config credentialConfig_input,
                                uh::licensing::license_activate_config license_activate_input);

    /**
     *
     * @return if the license key and activation could be validated
     */
    bool valid() override;

private:
    uh::licensing::license_activate_config m_license_activate;
};

} // namespace uh::licensing

#endif //CORE_CHECK_ONLINE_LICENSE_H
