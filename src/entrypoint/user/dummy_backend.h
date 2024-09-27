#ifndef CORE_ENTRYPOINT_USER_DUMMY_BACKEND_H
#define CORE_ENTRYPOINT_USER_DUMMY_BACKEND_H

#include "backend.h"

namespace uh::cluster::ep::user {

/**
 * Return a constant secret key for all users.
 */
class dummy_backend : public backend {
public:
    coro<user> find(std::string_view) override;

    static constexpr const char* SECRET_ACCESS_KEY = "secret";
};

} // namespace uh::cluster::ep::user

#endif
