#ifndef ENTRYPOINT_MULTIPART_STATE_H
#define ENTRYPOINT_MULTIPART_STATE_H

#include "common/db/db.h"
#include "common/types/common_types.h"
#include "common/utils/pool.h"

#include <map>

namespace uh::cluster {

struct upload_info {
    struct part {
        std::string etag;
        std::size_t size;
        address addr;
    };

    std::string key;
    std::string bucket;
    bool erased;
    std::map<uint16_t, part> parts;

    size_t effective_size{0};
    size_t data_size{0};

    [[nodiscard]] address generate_total_address() const {
        address addr;
        for (const auto& p : parts) {
            addr.append(p.second.addr);
        }

        return addr;
    }
};

class multipart_state {
public:
    multipart_state(boost::asio::io_context& ioc, const db::config& cfg);

    /**
     * Insert a new multipart upload and retrieve it's id.
     */
    coro<std::string> insert_upload(std::string bucket, std::string object_key);

    /**
     * Retrieve a pointer to the upload info for a given id.
     */
    coro<upload_info> details(const std::string& id);

    /**
     * Set a part info for a given id.
     */
    coro<void> append_upload_part_info(const std::string& id, uint16_t part_id,
                                       const dedupe_response& resp,
                                       size_t data_size, std::string&& md5);

    /**
     * Delete an upload with a given id
     */
    coro<void> remove_upload(const std::string& id);

    /**
     * Return a mapping of upload-id to object key for a given bucket.
     */
    coro<std::map<std::string, std::string>>
    list_multipart_uploads(const std::string& bucket);

private:
    pool<db::connection> m_db;

    /// Default grace period for deleted entries in seconds.
    static constexpr auto DEFAULT_TIMEOUT = 300;

    coro<void> clear_infos(db::connection& conn);
};

} // namespace uh::cluster

#endif
