#include "module.h"

namespace uh::cluster::ep::policy {

action module::check(const http_request& request, const command& cmd) {
    // allow execution of every request to not disrupt ongoing development
    return action::allow;
}

} // namespace uh::cluster::ep::policy
