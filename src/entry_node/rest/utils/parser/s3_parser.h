#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <entry_node/rest/http/http_request.h>
#include "entry_node/rest/utils/containers/internal_server_state.h"


namespace uh::cluster::rest::utils::parser
{

//------------------------------------------------------------------------------

    namespace b_http = boost::beast::http;
    namespace http = rest::http;

    class s3_parser
    {
    private:
        const b_http::request_parser<b_http::empty_body>& m_recv_req;
        utils::state& m_internal_server_state;

    public:
        s3_parser
        (const b_http::request_parser<b_http::empty_body>&, utils::state&);

        [[nodiscard]] std::unique_ptr<rest::http::http_request>
        parse() const;

    };

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
