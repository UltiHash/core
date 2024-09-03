#include "dummy_backend.h"

namespace uh::cluster::ep::user {

user dummy_backend::find(std::string_view) {
    return {.secret_key = SECRET_ACCESS_KEY};
}

} // namespace uh::cluster::ep::user
