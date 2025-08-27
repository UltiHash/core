#pragma once

#include "cache.h"
#include "object.h"

#include <string>

namespace uh::cluster {

class storage_cache {
public:
    using cache = cache<object_metadata, object_handle>;
    using global_data_view = storage::global::global_data_view;
    using stream = ep::http::stream;
    using body = ep::http::body;

    coro<void> send_object_as_response(object_handle object, stream& upstream);

    static storage_cache create(global_data_view storage) {
        return std::make_unique<lru_cache>()c, storage);
    }

    coro<void> put(const object_metadata& key, std::string tag,
                   std::shared_ptr<object_handle> value) {
        auto size = get_content_length(resp);
        // update cache
        total_size -= c.remove(path);
        auto required_size = total_size + size - storage_size;
        if (required_size > 0) {
            auto evicted = c.evict(required_size);
            auto address = concat(evicted);
            co_await storage.unlink(address);
        }
        m_cache->put(key, std::move(value));
    }

private:
    // TODO: Add total size limit and update it using mutex lock
    std::unique_ptr<cache> m_cache;
    global_data_view& m_storage;

    storage_cache(cache& c, global_data_view& storage)
        : m_cache(c),
          m_storage(storage) {}
};

} // namespace uh::cluster
