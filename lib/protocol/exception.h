#ifndef PROTOCOL_EXCEPTION_H
#define PROTOCOL_EXCEPTION_H

#include <utils/exception.h>


namespace uh::protocol
{

// ---------------------------------------------------------------------

DEFINE_EXCEPTION(read_error);
DEFINE_EXCEPTION(write_limit_exceeded);

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
