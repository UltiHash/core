#include "module.h"

namespace uh::cluster::ep::policy {

/*
 * This function implements policy evaluation logic
 * (see
 * https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_evaluation-logic.html#policy-eval-denyallow
 *  especially the flow chart)
 */
action module::check(const http_request& request, const command& cmd) const {
    // TODO set to deny when policies have been implemented completely
    action rv = action::allow;

    for (const auto& policy : m_policies) {
        if (auto result = policy.check(request, cmd); result) {
            switch (*result) {
            case action::deny:
                return action::deny;
            case action::allow:
                rv = action::allow;
                continue;
            }

            throw std::runtime_error("unsupported policy action");
        }
    }

    return rv;
}

} // namespace uh::cluster::ep::policy
