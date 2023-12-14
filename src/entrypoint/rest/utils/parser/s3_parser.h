#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include "../../http/http_request.h"
#include "../state/server_state.h"


namespace uh::cluster::rest::utils::parser
{

//------------------------------------------------------------------------------

    namespace b_http = boost::beast::http;
    namespace http = rest::http;

    class s3_parser
    {
    private:
        const b_http::request_parser<b_http::empty_body>& m_recv_req;
        utils::server_state& m_server_state;

    public:
        s3_parser
        (const b_http::request_parser<b_http::empty_body>&, utils::server_state&);

        [[nodiscard]] std::unique_ptr<rest::http::http_request>
        parse() const;

    };

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
