#ifndef CORE_ENTRYPOINT_HTTP_AUTH_BODY_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_AUTH_BODY_FACTORY_H

#include "entrypoint/user/backend.h"
#include "request_factory.h"

namespace uh::cluster::ep::http {

class auth_body_factory : public body_factory {
public:
    auth_body_factory(std::unique_ptr<user::backend> user_backend);

    std::unique_ptr<body> create(partial_parse_result& req) override;

private:
    std::unique_ptr<user::backend> m_user;
};

} // namespace uh::cluster::ep::http

#endif
