#ifndef REST_NODE_SRC_FUNCTIONS
#define REST_NODE_SRC_FUNCTIONS

namespace uh::rest
{

//------------------------------------------------------------------------------

    void putObject(const s3_request_object& s3_parsed_request)
    {
        std::cout << "putObject function called" << std::endl;
    }

//------------------------------------------------------------------------------

    void getObject(const s3_request_object& s3_parsed_request)
    {
        std::cout << "getObject function called" << std::endl;
    }

} // namespace uh::rest

#endif // REST_NODE_SRC_FUNCTIONS
