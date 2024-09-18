#include "dummy_backend.h"

namespace uh::cluster::ep::user {

coro<user> dummy_backend::find(std::string_view) {
    co_return user{.secret_key = SECRET_ACCESS_KEY};
}

} // namespace uh::cluster::ep::user
