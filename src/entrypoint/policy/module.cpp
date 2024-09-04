#include "module.h"

namespace uh::cluster::ep::policy {

/*
 * This function implements policy evaluation logic
 * (see
 * https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_evaluation-logic.html#policy-eval-denyallow
 *  especially the flow chart)
 */
action module::check(const http_request& request, const command& cmd) const {
    for (const auto& policy : m_policies) {
        auto result = policy.check(request, cmd);
        if (result.value_or(action::allow) == action::deny) {
            return action::deny;
        }
    }

    // TODO Does the requested resource have a resource-based policy?

    if (const auto& user : request.authenticated_user(); user) {
        for (const auto& policy : user->policies) {
            auto result = policy.check(request, cmd);
            if (result.value_or(action::deny) == action::allow) {
                return action::allow;
            }
        }
    }

    // TODO set to deny when policies have been implemented completely
    return action::allow;
}

} // namespace uh::cluster::ep::policy
