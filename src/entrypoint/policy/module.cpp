#include "module.h"

#include "parser.h"

namespace uh::cluster::ep::policy {

module::module(directory& dir) :m_directory(dir) {}

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
        if (result.value_or(effect::allow) == effect::deny) {
            co_return effect::deny;
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

    // TODO set to deny when policies have been implemented completely
    co_return effect::allow;
}

} // namespace uh::cluster::ep::policy
