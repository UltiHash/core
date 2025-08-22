#pragma once

#include "body.h"

namespace uh::cluster::ep::http {

class string_body : public body {
public:
    string_body(std::string&& body);

    std::optional<std::size_t> length() const override;
    coro<std::span<const char>> read(std::size_t count) override;
    coro<void> consume() override;

    const std::string& get_body() const { return m_body; }

    std::size_t buffer_size() const override;
private:
    std::string m_body;
    std::size_t m_read;
};

} // namespace uh::cluster::ep::http
