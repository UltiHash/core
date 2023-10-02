#pragma once

#include <boost/beast/http.hpp>
#include <entry_node/rest/http/http_request.h>


namespace uh::cluster::rest::utils::parser
{

//------------------------------------------------------------------------------

    namespace http = boost::beast::http;

    class s3_parser
    {
    private:
        const http::request_parser<http::empty_body>& m_recv_req;

    public:
        explicit s3_parser
        (const http::request_parser<http::empty_body>& recv_req);

        [[nodiscard]] std::unique_ptr<rest::http::http_request>
        parse() const;

    };

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser

