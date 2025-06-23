#include "garbage_collector.h"

namespace uh::cluster::ep {

coro<void> garbage_collector::collect(directory& dir,
                                      storage::global::global_data_view& gdv) {

    auto to_delete = co_await dir.next_deleted();
    while (to_delete) {
        try {
            auto freed = co_await gdv.unlink(to_delete->addr);
            LOG_DEBUG() << "reclaimed " << freed
                        << " bytes of data by disposing object "
                        << to_delete->id;
        } catch (const std::exception& e) {
            LOG_WARN() << "deleting of object " << to_delete->id
                       << " failed: " << e.what();
        }

        co_await dir.remove_object(to_delete->id);
        to_delete = co_await dir.next_deleted();
    }
}

} // namespace uh::cluster::ep
