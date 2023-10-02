#include <iostream>
#include "s3_parser.h"
#include <entry_node/rest/http/models/put_object.h>
#include <memory>

namespace uh::cluster::rest::utils::parser {

//------------------------------------------------------------------------------

    s3_parser::s3_parser
    (const http::request_parser<http::empty_body>& recv_req) : m_recv_req(recv_req)
    {}

    std::unique_ptr<rest::http::http_request>
    s3_parser::parse() const
    {
        if (m_recv_req.get().base().version() != 11)
        {
            throw std::runtime_error("bad http version. support exists only for HTTP 1.1.\n");
        }

        auto target = m_recv_req.get().base().target();
        auto method = m_recv_req.get().base().method();

        switch (method)
        {
            case boost::beast::http::verb::put:
                if (!target.empty() && (target.find('?') == std::string::npos))
                {
                    return std::make_unique<rest::http::model::put_object>(m_recv_req);
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            default:
                throw std::runtime_error("bad http verb.");
        }

    }

//------------------------------------------------------------------------------

} // uh::cluster
