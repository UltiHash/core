#ifndef CORE_ENTRYPOINT_USER_USER_H
#define CORE_ENTRYPOINT_USER_USER_H

#include <string>

namespace uh::cluster::ep::user {

struct user {
    std::string secret_key;
};

} // namespace uh::cluster::ep::user

#endif
