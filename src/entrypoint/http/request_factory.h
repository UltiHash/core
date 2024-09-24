#ifndef CORE_ENTRYPOINT_HTTP_AUTH_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_AUTH_REQUEST_FACTORY_H

#include "entrypoint/http/request.h"
#include "entrypoint/user/backend.h"
#include <boost/asio.hpp>

#include <memory>

namespace uh::cluster::ep::http {

class request_factory {
public:
    request_factory(std::unique_ptr<user::backend> users);

    coro<std::unique_ptr<request>> create(boost::asio::ip::tcp::socket&);

private:
    std::unique_ptr<user::backend> m_users;
};

} // namespace uh::cluster::ep::http

#endif
