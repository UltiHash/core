
#ifndef UH_CLUSTER_DEDUPLICATOR_INTERFACE_H
#define UH_CLUSTER_DEDUPLICATOR_INTERFACE_H

#include "common/types/address.h"

namespace uh::cluster {

struct deduplicator_interface {
    virtual coro<dedupe_response> deduplicate(const std::string_view& data) = 0;
    static constexpr role service_role = DEDUPLICATOR_SERVICE;
    virtual ~deduplicator_interface() = default;
};

} // namespace uh::cluster
#endif // UH_CLUSTER_DEDUPLICATOR_INTERFACE_H
