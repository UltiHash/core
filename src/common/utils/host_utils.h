
#ifndef UH_CLUSTER_HOST_UTILS_H
#define UH_CLUSTER_HOST_UTILS_H

#include "common.h"
#include <boost/asio.hpp>
#include <string>

namespace uh::cluster {

inline bool is_valid_ip(const std::string& ip) {
    try {
        auto address = boost::asio::ip::address::from_string(ip);
        return address.is_v4() || address.is_v6();
    } catch (const std::exception& e) {
        return false;
    }
}

inline std::string get_host() {
    const char* var_value = std::getenv(ENV_CFG_ENDPOINT_HOST);
    if (var_value == nullptr) {
        return boost::asio::ip::host_name();
    } else {
        if (is_valid_ip(var_value))
            return {var_value};
        else
            throw std::invalid_argument(
                "the environmental variable " +
                std::string(ENV_CFG_ENDPOINT_HOST) +
                " does not contain a valid IPv4 or IPv6 address: '" +
                std::string(var_value) + "'");
    }
}

} // namespace uh::cluster

#endif // UH_CLUSTER_HOST_UTILS_H
