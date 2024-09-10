#ifndef WAITER_RECOVERY_MODULE_H
#define WAITER_RECOVERY_MODULE_H
#include "recovery_module.h"

namespace uh::cluster {

struct recovery_module_factory;

class waiter_recovery_module : public recovery_module {
public:
    void async_check_recover(std::atomic<ec_status>& status) override {
        status = healthy;
    }

private:
    friend struct recovery_module_factory;
    waiter_recovery_module() = default;
};

} // end namespace uh::cluster

#endif // WAITER_RECOVERY_MODULE_H
