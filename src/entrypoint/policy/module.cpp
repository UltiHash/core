#include "module.h"

#include "common/telemetry/log.h"
#include "common/utils/misc.h"
#include "parser.h"

namespace uh::cluster::ep::policy {

namespace {

std::list<policy> read_global_policies(const std::filesystem::path& path) {
    std::list<policy> rv;

    if (std::filesystem::exists(path)) {
        rv = parser::parse(read_file(path));
        LOG_INFO() << "loaded " << rv.size() << " global policies from "
                   << path;
    }

    return rv;
}

} // namespace

const std::filesystem::path module::GLOBAL_CONFIG = "/etc/uh/policies.json";

module::module(directory& dir) :m_directory(dir),
    m_policies(read_global_policies(GLOBAL_CONFIG)) {}

/*
 * This function implements policy evaluation logic
 * (see
 * https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_evaluation-logic.html#policy-eval-denyallow
 *  especially the flow chart)
 */
coro<effect> module::check(const http::request& request,
                           const command& cmd) const {
    for (const auto& policy : m_policies) {
        auto result = policy.check(request, cmd);
        if (result.value_or(effect::deny) == effect::allow) {
            co_return effect::allow;
        }
    }

    if (auto resource =
            co_await m_directory.get_bucket_policy(request.bucket());
        resource) {
        // TODO cache bucket policies
        auto policies = parser::parse(*resource);
        for (const auto& p : policies) {
            if (p.check(request, cmd).value_or(effect::deny) == effect::allow) {
                co_return effect::allow;
            }
        }
    }

    for (const auto& policy : request.authenticated_user().policies) {
        auto result = policy.check(request, cmd);
        if (result.value_or(effect::deny) == effect::allow) {
            co_return effect::allow;
        }
    }

    co_return effect::deny;
}

} // namespace uh::cluster::ep::policy
