#ifndef CORE_ENTRYPOINT_USER_BACKEND_H
#define CORE_ENTRYPOINT_USER_BACKEND_H

#include "common/types/common_types.h"
#include "user.h"

namespace uh::cluster::ep::user {

class backend {
public:
    virtual ~backend() = default;

    /**
     * Find a user record by passing a string-based user reference.
     * @throw if user cannot be found
     */
    virtual coro<user> find(std::string_view) = 0;
};

} // namespace uh::cluster::ep::user

#endif
