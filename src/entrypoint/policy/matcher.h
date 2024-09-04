#ifndef CORE_ENTRYPOINT_POLICY_MATCHER_H
#define CORE_ENTRYPOINT_POLICY_MATCHER_H

#include <functional>
#include <set>
#include <string>

namespace uh::cluster {
class http_request;
class command;
} // namespace uh::cluster

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
