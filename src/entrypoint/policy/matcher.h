#ifndef CORE_ENTRYPOINT_POLICY_MATCHER_H
#define CORE_ENTRYPOINT_POLICY_MATCHER_H

#include "entrypoint/commands/command.h"
#include "entrypoint/http/http_request.h"
#include <functional>
#include <set>

namespace uh::cluster::ep::policy {

typedef std::function<bool(const http_request&, const command&)> matcher;

inline matcher match_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const http_request&,
                                          const command& cmd) { return false; };
}

inline matcher match_not_action(std::set<std::string> actions) {
    return [actions = std::move(actions)](const http_request&,
                                          const command& cmd) { return false; };
}

inline matcher match_resource(std::set<std::string> resources) {
    return [actions = std::move(resources)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher match_not_resource(std::set<std::string> resources) {
    return [actions = std::move(resources)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher match_principal(std::set<std::string> principals) {
    return [actions = std::move(principals)](
               const http_request&, const command& cmd) { return false; };
}

inline matcher match_not_principal(std::set<std::string> principals) {
    return [actions = std::move(principals)](
               const http_request&, const command& cmd) { return false; };
}

} // namespace uh::cluster::ep::policy

#endif
