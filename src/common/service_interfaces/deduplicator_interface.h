#pragma once

#include <common/telemetry/context.h>
#include <common/types/common_types.h>
#include <common/utils/common.h>

namespace uh::cluster {

struct deduplicator_interface {
    virtual coro<dedupe_response> deduplicate(context& ctx,
                                              std::string_view data) = 0;
    static constexpr role service_role = DEDUPLICATOR_SERVICE;
    virtual ~deduplicator_interface() = default;
};

} // namespace uh::cluster
