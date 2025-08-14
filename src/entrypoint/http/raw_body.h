#pragma once

#include "body.h"
#include "raw_request.h"
#include "stream.h"

namespace uh::cluster::ep::http {

class raw_body : public body {
public:
    raw_body(stream& s, raw_request& req);

    std::optional<std::size_t> length() const override;

    coro<std::span<const char>> read(std::size_t count) override;
    coro<void> consume() override;

    std::size_t buffer_size() const override;

private:
    stream& m_s;
    std::size_t m_length;
};

} // namespace uh::cluster::ep::http
