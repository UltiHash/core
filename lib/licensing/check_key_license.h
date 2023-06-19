//
// Created by benjamin-elias on 01.06.23.
//

#ifndef CORE_CHECK_AIRGAP_LICENSE_H
#define CORE_CHECK_AIRGAP_LICENSE_H

#include "licensing/check_license.h"

namespace uh::licensing
{

class check_key_license: public check_license
{

public:

    /*
     * license model of online activation and air-gap use without time limit
     */
    explicit check_key_license(uh::licensing::license_config license_config,
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

} // uh::licensing

#endif //CORE_CHECK_AIRGAP_LICENSE_H
