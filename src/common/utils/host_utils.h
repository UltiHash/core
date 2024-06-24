
#ifndef UH_CLUSTER_HOST_UTILS_H
#define UH_CLUSTER_HOST_UTILS_H

#include "common.h"
#include <boost/asio.hpp>
#include <string>

namespace uh::cluster {

bool is_valid_ip(const std::string& ip);

std::string get_host();

} // namespace uh::cluster

#endif // UH_CLUSTER_HOST_UTILS_H
