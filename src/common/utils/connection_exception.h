#pragma once

#include <boost/system/system_error.hpp>
#include <exception>
#include <sstream>
#include <string>

namespace uh::cluster {

class connection_exception : public std::exception {
public:
    enum class origin { upstream, downstream };

    connection_exception(origin org, const std::string& msg,
                         const boost::system::system_error& original)
        : m_origin(org),
          m_message(msg),
          m_original(original) {
        std::ostringstream oss;
        oss << "[" << (org == origin::upstream ? "upstream" : "downstream")
            << "] " << msg << " | boost error: " << original.what();
        m_full_message = oss.str();
    }

    const char* what() const noexcept override {
        return m_full_message.c_str();
    }

    origin get_origin() const noexcept { return m_origin; }
    const boost::system::system_error& original_exception() const noexcept {
        return m_original;
    }
    const std::string& message() const noexcept { return m_message; }

private:
    origin m_origin;
    std::string m_message;
    boost::system::system_error m_original;
    std::string m_full_message;
};

} // namespace uh::cluster
