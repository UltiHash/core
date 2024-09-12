#ifndef RECOVERY_MODULE_H
#define RECOVERY_MODULE_H

namespace uh::cluster {

enum ec_status {
    empty,
    degraded,
    recovering,
    healthy,
    failed_recovery,
};

struct recovery_module {
    virtual void async_check_recover(std::atomic<ec_status>&) = 0;
    virtual ~recovery_module() = default;
};

} // end namespace uh::cluster

#endif // RECOVERY_MODULE_H
