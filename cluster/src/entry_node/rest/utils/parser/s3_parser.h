#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/http/models/multi_part_container.h>
#include <entry_node/rest/utils/containers/ts_unordered_map.h>

namespace uh::cluster::rest::utils::parser
{

//------------------------------------------------------------------------------

    namespace http = boost::beast::http;

    class s3_parser
    {
    private:
        const http::request_parser<http::empty_body>& m_recv_req;
        rest::utils::ts_unordered_map<std::string, std::shared_ptr<rest::http::model::multi_part_container>>& m_uomap_multipart;

    public:
        s3_parser
        (const http::request_parser<http::empty_body>& recv_req,
         rest::utils::ts_unordered_map<std::string, std::shared_ptr<rest::http::model::multi_part_container>>&);

        [[nodiscard]] std::unique_ptr<rest::http::http_request>
        parse() const;

    };

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser

