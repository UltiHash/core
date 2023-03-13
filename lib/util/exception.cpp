#include "exception.h"

#include <system_error>
#include <errno.h>
#include <sstream>


namespace uh::util
{

namespace
{

// ---------------------------------------------------------------------

std::string make_message(const std::string& file, std::size_t line,
                         const std::string& message)
{
    std::stringstream s;

#ifdef DEBUG
    s << file << ":" << line << " ";
#endif

    s << message;

    return s.str();
}

// ---------------------------------------------------------------------

}

// ---------------------------------------------------------------------

exception::exception(const std::string& file, std::size_t line,
                     const std::string& message)
    : m_message(make_message(file, line, message))
{
}

// ---------------------------------------------------------------------

const char* exception::what() const noexcept
{
    return m_message.c_str();
}

// ---------------------------------------------------------------------

[[noreturn]] void throw_from_syserror(const std::string& file, std::size_t line)
{
    auto cond = std::generic_category().default_error_condition(errno);

    if (errno == EEXIST)
    {
        throw file_exists(file, line, cond.message());
    }

    throw system_error(file, line, cond.message());
}

// ---------------------------------------------------------------------

} // namespace uh::util
