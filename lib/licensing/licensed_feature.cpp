//
// Created by benjamin-elias on 01.06.23.
//

#include "licensed_feature.h"


namespace uh::licensing{

    // ---------------------------------------------------------------------

    licensed_feature::licensed_feature(std::string feature_name,bool is_active):
    feature_name(std::move(feature_name)), state(is_active){}

    const std::string &licensed_feature::getFeatureName() const {
        return feature_name;
    }

    // ---------------------------------------------------------------------

}
