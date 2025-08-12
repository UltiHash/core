#pragma once

#include "body.h"

namespace uh::cluster::ep::http {

class string_body : public body {
public:
    string_body(std::string&& body);

    std::optional<std::size_t> length() const override;
    coro<std::size_t> read(std::span<char>) override;

    const std::string& get_body() const { return m_body; }
    std::vector<boost::asio::const_buffer>
    get_raw_buffer() const override {
        return {boost::asio::buffer(m_body)};
    }

private:
    std::string m_body;
};

} // namespace uh::cluster::ep::http
