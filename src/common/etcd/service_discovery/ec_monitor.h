//
// Created by massi on 8/1/24.
//

#ifndef MAINTAINER_MONITOR_H
#define MAINTAINER_MONITOR_H
#include "../namespace.h"

namespace uh::cluster {

class ec_group_maintainer;
enum ec_status {
    degraded,
    healthy,
    recovering,
};

struct ec_monitor {

    friend ec_group_maintainer;

private:
    virtual void status_update(size_t gid, ec_status status);

protected:
    virtual ~ec_monitor() = default;
};
} // namespace uh::cluster

#endif // MAINTAINER_MONITOR_H
