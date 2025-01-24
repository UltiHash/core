#pragma once

#include <common/types/common_types.h>

#include <entrypoint/http/request.h>

#include <memory>

namespace uh::cluster::ep::http {

class no_auth {
public:
    static coro<std::unique_ptr<request>>
    create(boost::asio::ip::tcp::socket& s, raw_request req);
};

} // namespace uh::cluster::ep::http
