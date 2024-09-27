#ifndef CORE_ENTRYPOINT_HTTP_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_REQUEST_FACTORY_H

#include "entrypoint/http/request.h"
#include "entrypoint/user/db.h"
#include <boost/asio.hpp>

#include <memory>

namespace uh::cluster::ep::http {

class request_factory {
public:
    request_factory(user::db&& users);

    coro<std::unique_ptr<request>> create(boost::asio::ip::tcp::socket&);

private:
    user::db m_users;
};

} // namespace uh::cluster::ep::http

#endif
