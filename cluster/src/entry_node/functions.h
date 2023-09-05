#ifndef REST_NODE_SRC_FUNCTIONS
#define REST_NODE_SRC_FUNCTIONS

#include "s3_parser.h"

namespace uh::rest
{

//------------------------------------------------------------------------------

    void putObject(const parsed_request_wrapper& s3_parsed_request)
    {
        std::cout << "putObject function called" << std::endl;
    }

//------------------------------------------------------------------------------

    void getObject(const parsed_request_wrapper& s3_parsed_request)
    {
        std::cout << "getObject function called" << std::endl;
    }

} // namespace uh::cluster

#endif // REST_NODE_SRC_FUNCTIONS
