#pragma once

#include "entrypoint/http/request.h"
#include "entrypoint/user/db.h"
#include <boost/asio.hpp>

#include <memory>

namespace uh::cluster::ep::http {

class request_factory {
public:
    request_factory(user::db& users);

    notrace_coro<std::unique_ptr<request>>
    create(boost::asio::ip::tcp::socket& sock, raw_request& req);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::http
