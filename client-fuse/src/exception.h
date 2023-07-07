#ifndef CLIENT_FUSE_EXCEPTION_H
#define CLIENT_FUSE_EXCEPTION_H

#include <util/exception.h>


namespace uh::fuse
{

// ---------------------------------------------------------------------

class exception : public util::exception
{
public:
    exception(const std::string& file, std::size_t line, const std::string& msg, int error)
        : util::exception(file, line, msg),
          m_error(error)
    {
    }

    int error() const
    {
        return m_error;
    }

private:
    int m_error;
};

// ---------------------------------------------------------------------

} // namespace uh::fuse

#endif
