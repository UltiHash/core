//
// Created by benjamin-elias on 17.06.23.
//

#include "licensing/mod.h"
#include <memory>

//module;

//export module global_licensing_data_node;

namespace uh::dbn::licensing{

inline static std::unique_ptr<uh::dbn::licensing::mod> global_license_pointer;

}