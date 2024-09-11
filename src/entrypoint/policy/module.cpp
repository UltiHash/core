#include "module.h"

namespace uh::cluster::ep::policy {

/*
 * This function implements policy evaluation logic
 * (see
 * https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_evaluation-logic.html#policy-eval-denyallow
 *  especially the flow chart)
 */
effect module::check(const http::request& request, const command& cmd) const {
    for (const auto& policy : m_policies) {
        auto result = policy.check(request, cmd);
        if (result.value_or(effect::allow) == effect::deny) {
            return effect::deny;
        }
    }

    // TODO Does the requested resource have a resource-based policy?

    for (const auto& policy : request.authenticated_user().policies) {
        auto result = policy.check(request, cmd);
        if (result.value_or(effect::deny) == effect::allow) {
            return effect::allow;
        }
    }

    // TODO set to deny when policies have been implemented completely
    return effect::allow;
}

} // namespace uh::cluster::ep::policy
