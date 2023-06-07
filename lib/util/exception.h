#ifndef UTILS_EXCEPTION_H
#define UTILS_EXCEPTION_H

#include <exception>
#include <string>


#define FILELINE __FILE__, __LINE__

#define DEFINE_SUB_EXCEPTION(name, base) class name : public base               \
    {                                                                           \
    public:                                                                     \
        name(const std::string& file, std::size_t line, const std::string& msg) \
            : base(file, line, msg) {}                                          \
    }

#define DEFINE_EXCEPTION(name) DEFINE_SUB_EXCEPTION(name, uh::util::exception)

#define THROW(exception, message) throw exception(FILELINE, message)
#define THROW_FROM_ERRNO() uh::util::throw_from_syserror(FILELINE)

namespace uh::util
{

// ---------------------------------------------------------------------

class exception : public std::exception
{
public:
    exception(const std::string& file, std::size_t line,
              const std::string& message);

    [[nodiscard]] const char* what() const noexcept override;

private:
    std::string m_message;
};

// ---------------------------------------------------------------------

DEFINE_EXCEPTION(illegal_args);
DEFINE_EXCEPTION(illegal_state);
DEFINE_EXCEPTION(system_error);
DEFINE_EXCEPTION(no_space_error);
DEFINE_SUB_EXCEPTION(file_exists, system_error);

// ---------------------------------------------------------------------

[[noreturn]] void throw_from_syserror(const std::string& file, std::size_t line);

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
