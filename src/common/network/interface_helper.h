//
// Created by max on 07.02.24.
//

#ifndef UH_CLUSTER_INTERFACE_HELPER_H
#define UH_CLUSTER_INTERFACE_HELPER_H

#include <string>
#include <vector>
#include <exception>
#include <fstream>
#include <sstream>

#include <netdb.h>
#include <sys/stat.h>
#include <ifaddrs.h>

namespace uh::cluster {

class interface_helper {

    // the interface_helper class is currently Linux-specific and thus platform-dependent.
    // for other POSIX compliant OSes, support can be added when/if needed

    public:
        static std::string retrieve_ip_address();
        interface_helper() = delete;
        ~interface_helper() = delete;
        interface_helper(const interface_helper&) = delete;
        interface_helper(interface_helper&&) = delete;
        interface_helper& operator=(const interface_helper&) = delete;
        interface_helper& operator=(interface_helper&&) = delete;

    private:
        struct route_info {
            std::string iface;
            std::string destination;
            std::string gateway;
            std::string flags;
            std::string refcnt;
            std::string use;
            std::string metric;
            std::string mask;
            std::string mtu;
            std::string window;
            std::string irtt;
        };

        static std::vector<route_info> get_route_info();
        static bool is_wireless_interface(const std::string& iface);
        static std::string get_default_route_iface();
};

}

#endif //UH_CLUSTER_INTERFACE_HELPER_H
