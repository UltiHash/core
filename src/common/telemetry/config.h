#ifndef COMMON_TELEMETRY_CONFIG_H
#define COMMON_TELEMETRY_CONFIG_H

#include <string>

namespace uh::cluster
{

struct telemetry_config {
    std::string endpoint;
    unsigned interval = 1000;
};

} // namespace uh::cluster

#endif
