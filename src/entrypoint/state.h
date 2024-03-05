#ifndef ENTRYPOINT_STATE_H
#define ENTRYPOINT_STATE_H

#include "common/types/common_types.h"
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace uh::cluster {

struct upload_info {
    size_t effective_size{0};
    size_t data_size{0};
    std::map<uint16_t, std::string> etags;
    std::map<uint16_t, size_t> part_sizes;
    std::map<uint16_t, address> addresses;
    unsigned long long upload_init_time{0};

    std::string key;
    std::string bucket;

    [[nodiscard]] address generate_total_address() const {
        address addr;
        for (const auto& a : addresses) {
            addr.append_address(a.second);
        }
        return addr;
    }
};

struct upload_state {
    std::string insert_upload(std::string bucket, std::string object_key);
    bool contains_upload(const std::string& id) const;
    std::shared_ptr<upload_info> get_upload_info(const std::string& id) const;
    void append_upload_part_info(const std::string& id, uint16_t part_id,
                                 const dedupe_response& resp,
                                 const std::string& data);

    void remove_upload(const std::string& id);

    std::map<std::string, std::string>
    list_multipart_uploads(const std::string&) const;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<upload_info>> m_infos;
};

struct state {
    struct upload_state m_uploads;
};

} // namespace uh::cluster

#endif
