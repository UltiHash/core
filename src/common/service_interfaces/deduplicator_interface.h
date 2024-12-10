#ifndef CORE_COMMON_SERVICE_INTERFACES_DEDUPLICATOR_INTERFACE_H
#define CORE_COMMON_SERVICE_INTERFACES_DEDUPLICATOR_INTERFACE_H

#include "common/telemetry/context.h"
#include "common/utils/common.h"

namespace uh::cluster {

struct deduplicator_interface {
    virtual coro<dedupe_response> deduplicate(context& ctx,
                                              const std::string_view& data) = 0;
    static constexpr role service_role = DEDUPLICATOR_SERVICE;
    virtual ~deduplicator_interface() = default;
};

} // namespace uh::cluster

#endif
