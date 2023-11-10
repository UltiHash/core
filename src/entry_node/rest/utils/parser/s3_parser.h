#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/containers/ts_unordered_map.h>
#include <entry_node/rest/utils/containers/ts_map.h>

namespace uh::cluster::rest::utils::parser
{

//------------------------------------------------------------------------------

    namespace b_http = boost::beast::http;
    namespace http = rest::http;

    class s3_parser
    {
    private:
        const b_http::request_parser<b_http::empty_body>& m_recv_req;
        rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>>>& m_uomap_multipart;

    public:
        s3_parser
        (const b_http::request_parser<b_http::empty_body>& recv_req,
         rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>>>&);

        [[nodiscard]] std::unique_ptr<rest::http::http_request>
        parse() const;

    };

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
