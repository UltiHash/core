#include "error.h"

#include <ostream>
#include <vector>


namespace uh::cluster
{

namespace
{

static const std::vector<std::string> error_messages =
{
    "success",
    "unknown",
    "bucket does not exists",
    "object does not exists",
    "bucket is not empty"
};

static const std::string error_out_of_range = "error out of range";

}

error::error(type t, const std::string& message)
    : m_type(t),
      m_message(message) {
}

error::error(uint32_t t, const std::string& message)
    : error(static_cast<type>(t), message) {
}

const std::string& error::message() const {
    if (!m_message.empty()) {
        return m_message;
    }

    auto ec = code();
    if (error_messages.size() <= ec) {
        return error_out_of_range;
    }

    return error_messages[ec];
}

uint32_t error::code() const {
    return static_cast<uint32_t>(m_type);
}

error::type error::operator*() const
{
    return m_type;
}

const char* error_exception::what() const noexcept {
    return m_error.message().c_str();
}

std::ostream& operator<<(std::ostream& out, const error& e) {
    out << e.message() << " [" << e.code() << "]";
    return out;
}

std::ostream& operator<<(std::ostream& out, const error_exception& e) {
    out << e.error();
    return out;
}

}
