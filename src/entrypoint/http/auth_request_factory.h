#ifndef CORE_ENTRYPOINT_HTTP_AUTH_REQUEST_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_AUTH_REQUEST_FACTORY_H

#include "entrypoint/user/backend.h"
#include "request_factory.h"

#include <memory>

namespace uh::cluster::ep::http {

class auth_request_factory : public request_factory {
public:
    auth_request_factory(std::unique_ptr<user::backend> users);

    coro<std::unique_ptr<request>>
    create(boost::asio::ip::tcp::socket&) override;

private:
    std::unique_ptr<user::backend> m_users;
};

} // namespace uh::cluster::ep::http

#endif
