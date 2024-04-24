
#ifndef UH_CLUSTER_HOST_UTILS_H
#define UH_CLUSTER_HOST_UTILS_H

#include "common.h"
#include <string>
#include <boost/asio.hpp>

namespace uh::cluster {


bool is_valid_ip(const std::string& ip);


std::string get_host();

} // namespace

#endif // UH_CLUSTER_HOST_UTILS_H
