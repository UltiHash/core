#pragma once

#include "body.h"
#include "common/utils/common.h"
#include "raw_request.h"

#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace uh::cluster::ep::http {

class chunked_body : public ep::http::body {
public:
    enum class trailing_headers { none, read };

    chunked_body(boost::asio::ip::tcp::socket& sock, raw_request& req,
                 trailing_headers trailing = trailing_headers::none);

    struct chunk_header {
        std::size_t size;
        std::map<std::string_view, std::string_view> extensions;
        std::string extensions_string;
    };

    virtual ~chunked_body() = default;

    std::optional<std::size_t> length() const override;
    coro<std::size_t> read(std::span<char> dest) override;

    std::vector<boost::asio::const_buffer>
    get_raw_buffer() const override {
        return m_raw_buffers;
    }

    virtual void on_chunk_header(const chunk_header&);
    virtual void on_chunk_data(std::span<char>);
    virtual void on_chunk_done();
    virtual void on_body_done();

private:
    coro<void> read_nl();
    std::size_t find_nl() const;
    coro<chunk_header> read_chunk_header();
    coro<std::size_t> read_data(std::span<char> buffer);

    static constexpr std::size_t BUFFER_SIZE = MEBI_BYTE;

    boost::asio::ip::tcp::socket& m_socket;
    std::vector<char> m_buffer;
    trailing_headers m_trailing;
    std::size_t m_read_position = 0;
    std::size_t m_chunk_bytes_left = 0ull;
    bool m_end = false;
    std::vector<boost::asio::const_buffer> m_raw_buffers;
};

} // namespace uh::cluster::ep::http
