#pragma once

#include "common.h"
#include <boost/asio.hpp>
#include <string>

namespace uh::cluster {

bool is_valid_ip(const std::string& ip);

std::string get_host();

} // namespace uh::cluster
