//
// Created by max on 07.02.24.
//

#include "interface_helper.h"

namespace uh::cluster {

    std::vector<interface_helper::route_info> interface_helper::get_route_info() {
        std::vector<interface_helper::route_info> routes;
        std::ifstream route_file("/proc/net/route");
        if (!route_file.is_open()) {
            throw std::runtime_error("failed to open /proc/net/route");
            return routes;
        }

        // Discard the header line
        std::string line;
        std::getline(route_file, line);

        // Parse each line in the file
        while (std::getline(route_file, line)) {
            std::istringstream iss(line);
            route_info route;
            iss >> route.iface >> route.destination >> route.gateway >> route.flags >> route.refcnt
                >> route.use >> route.metric >> route.mask >> route.mtu >> route.window >> route.irtt;
            routes.push_back(route);
        }

        route_file.close();
        return routes;
    }

    bool interface_helper::is_wireless_interface(const std::string& iface) {
        struct stat buffer;
        std::string path = "/sys/class/net/" + iface + "/wireless";
        return (stat(path.c_str(), &buffer) == 0);
    }

    std::string interface_helper::get_default_route_iface() {
        for (const auto& route : get_route_info()) {
            if (route.destination == "00000000" && route.mask == "00000000" && !is_wireless_interface(route.iface)) {
                return route.iface;
            }
        }
        throw std::runtime_error("no default route interface could be identified");
    }

    std::string interface_helper::retrieve_ip_address() {
        struct ifaddrs *ifaddr, *ifa;
        int family, s;
        auto default_iface = get_default_route_iface();
        char host[NI_MAXHOST];

        if (getifaddrs(&ifaddr) == -1) {
            throw std::runtime_error("error getting network interface addresses");
        }

        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr)
                continue;

            family = ifa->ifa_addr->sa_family;

            if (family == AF_INET && std::string_view(ifa->ifa_name) == default_iface) {
                s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
                if (s != 0) {
                    throw std::runtime_error("parsing IP address failed");
                }
                return {host};
            }
        }

        throw std::runtime_error("unable to determine IP address of default network interface");
    }

}
