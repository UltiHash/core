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
        if (auto result = policy.check(request, cmd); result) {
            if (*result == action::deny) {
                return action::deny;
            }
        }
    }

    // allow execution of every request to not disrupt ongoing development
    return action::allow;
}

} // namespace uh::cluster::ep::policy
