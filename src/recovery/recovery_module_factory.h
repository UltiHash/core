#ifndef RECOVERY_MODULE_FACTORY_H
#define RECOVERY_MODULE_FACTORY_H
#include "common/telemetry/metrics.h"
#include "performer_recovery_module.h"
#include "waiter_recovery_module.h"

namespace uh::cluster {

struct recovery_module_factory {
    static std::unique_ptr<recovery_module>
    create(storage_service_get_handler& getter, boost::asio::io_context& ioc,
           ec_interface& ec_calc) {
        if (global_service_role == RECOVERY_SERVICE) {
            return std::unique_ptr<performer_recovery_module>(
                new performer_recovery_module(getter, ioc, ec_calc));
        }
        return std::unique_ptr<waiter_recovery_module>(
            new waiter_recovery_module);
    }
};

} // end namespace uh::cluster

#endif // RECOVERY_MODULE_FACTORY_H
