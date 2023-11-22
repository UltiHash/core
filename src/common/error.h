#ifndef CORE_COMMON_ERROR_H
#define CORE_COMMON_ERROR_H

#include <exception>
#include <iosfwd>
#include <string>
#include <utility>
#include <cstdint>

namespace uh::cluster {

class error {
public:
    enum type {
        success = 0,
        unknown = 1,
        bucket_not_found = 2,
        object_not_found = 3,
        bucket_not_empty = 4,
    };

    error(type t = unknown, const std::string& message = "");
    error(uint32_t t, const std::string& message = "");

    const std::string& message() const;
    uint32_t code() const;
    type operator*() const;

    static type from_code(uint32_t code);

private:
    std::string m_message;
    type m_type;
};

class error_exception : public std::exception {
public:
    error_exception(uh::cluster::error e = uh::cluster::error())
        : m_error(std::move(e)) {
    }

    const char* what() const noexcept override;

    const uh::cluster::error& error() const {
        return m_error;
    }

private:
    uh::cluster::error m_error;
};

std::ostream& operator<<(std::ostream& out, const error& e);
std::ostream& operator<<(std::ostream& out, const error_exception& e);

} // namespace uh::cluster

#endif
