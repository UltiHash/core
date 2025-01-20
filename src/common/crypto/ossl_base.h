#pragma once

#include <string>

namespace uh::cluster {

[[noreturn]] void throw_from_error(const std::string& prefix);

} // namespace uh::cluster
