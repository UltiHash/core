#ifndef CORE_ENTRYPOINT_POLICY_MATCHER_H
#define CORE_ENTRYPOINT_POLICY_MATCHER_H

#include "entrypoint/commands/command.h"
#include "entrypoint/http/http_request.h"
#include <functional>

namespace uh::cluster::ep::policy {

typedef std::function<bool(const http_request&, const command&)> matcher;

} // namespace uh::cluster::ep::policy

#endif
