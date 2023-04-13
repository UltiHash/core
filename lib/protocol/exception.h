#ifndef PROTOCOL_EXCEPTION_H
#define PROTOCOL_EXCEPTION_H

#include <util/exception.h>


namespace uh::protocol
{

// ---------------------------------------------------------------------

DEFINE_EXCEPTION(read_error);
DEFINE_EXCEPTION(write_limit_exceeded);
DEFINE_EXCEPTION(illegal_args);
DEFINE_EXCEPTION(unsupported);
DEFINE_EXCEPTION(internal_error);
DEFINE_EXCEPTION(server_busy);


// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
