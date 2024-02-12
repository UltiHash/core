#pragma once

#include <map>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include "URI.h"
#include "entrypoint/utils/hash.h"

namespace uh::cluster::entry
{
    namespace http = boost::beast::http;        // from <boost/beast/http.hpp>
    template <typename T>
    using coro =  boost::asio::awaitable <T>;   // for coroutine


    class http_request
    {
    public:

        http_request(const http::request_parser<http::empty_body>& req,
                      boost::asio::ip::tcp::socket& stream,
                      boost::beast::flat_buffer& buffer) :
        m_req(req),
        m_stream(stream),
        m_buffer(buffer),
        m_uri(req)
        {}

        inline const URI& get_URI() const {
            return m_uri;
        }

        inline const std::string& get_eTag() const {
            return m_etag;
        }

        inline const std::string& get_body() const {
            return m_body;
        }

        inline std::size_t get_body_size() const {
            return m_body.size();
        }

        inline method get_method() const {
            return m_uri.get_method();
        }

        coro<void> read_body()
        {
            if (m_req.get().has_content_length())
            {
                if (m_req.content_length().value() != 0)
                {
                    std::size_t content_length = m_req.content_length().value();
                    m_body.append(content_length, 0);

                    auto data_left = content_length - m_buffer.size();

                    // copy remaining bytes from flat buffer to body_buffer
                    boost::asio::buffer_copy(boost::asio::buffer(m_body), m_buffer.data());
                    auto size_transferred = co_await boost::asio::async_read(m_stream, boost::asio::buffer(m_body.data() + m_buffer.size(), data_left),
                                                                             boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                    if (size_transferred + m_buffer.size() != content_length)
                    {
                        throw std::runtime_error("error reading the http body");
                    }

                    MD5 md5 {};
                    m_etag = md5.calculateMD5(m_body);
                }
            }
            else
            {
                throw std::runtime_error("please specify the content length on requests as other methods without content length are currently not supported");
            }

            co_return;
        }

    private:
        const http::request_parser<http::empty_body>& m_req;
        boost::asio::ip::tcp::socket& m_stream;
        boost::beast::flat_buffer& m_buffer;

        URI m_uri;
        std::string m_etag {};
        std::string m_body {};
    };

} // uh::cluster::entry
