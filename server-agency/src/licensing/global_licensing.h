//
// Created by benjamin-elias on 17.06.23.
//

#include "licensing/mod.h"
#include <memory>

//module;

//export module global_licensing_agency_node;

namespace uh::an::licensing
{

static std::unique_ptr<uh::an::licensing::mod> global_license_pointer;

}