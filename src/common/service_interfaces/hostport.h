#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace uh::cluster {

struct hostport {
    std::string hostname{};
    uint16_t port{0};
    std::string to_string() const {
        return hostname + ":" + std::to_string(port);
    }
    static hostport create(std::string_view str) {
        size_t pos = str.rfind(':');
        if (pos == std::string_view::npos) {
            throw std::invalid_argument("Invalid hostport format, missing ':'");
        }

        std::string hostname(str.substr(0, pos));
        std::string_view port_str = str.substr(pos + 1);

        uint16_t port;
        try {
            unsigned long port_ul = std::stoul(std::string(port_str));
            if (port_ul > UINT16_MAX) {
                throw std::out_of_range("Port number exceeds uint16_t range");
            }
            port = static_cast<uint16_t>(port_ul);
        } catch (const std::exception& e) {
            throw std::invalid_argument("Invalid port number");
        }

        return {hostname, port};
    }
};

} // namespace uh::cluster
