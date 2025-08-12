#pragma once

#include <entrypoint/http/request_factory.h>

namespace uh::cluster::proxy {

class request_factory {
public:
    coro<std::unique_ptr<ep::http::request>> create(boost::asio::ip::tcp::socket& sock,
                                                    ep::http::raw_request& req);
};

} // namespace uh::cluster::proxy
