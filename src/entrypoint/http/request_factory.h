#pragma once

#include <entrypoint/http/request.h>
#include <entrypoint/http/stream.h>
#include <entrypoint/user/db.h>

#include <memory>

namespace uh::cluster::ep::http {

class request_factory {
public:
    request_factory(user::db& users);

    coro<std::unique_ptr<request>> create(stream& s, boost::asio::ip::tcp::endpoint peer);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::http
