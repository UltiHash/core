#pragma once

#include "common/types/common_types.h"
#include "entrypoint/utils/md5.h"
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

    [[nodiscard]] address generate_total_address() const {
        address addr;
        for (const auto& a : addresses) {
            addr.append_address(a.second);
        }
        return addr;
    }
};

struct upload_state {
    upload_state() = default;

    bool insert_upload(std::string upload_id, std::string bucket,
                       std::string object_key);
    bool contains_upload(const std::string& bucket) const;
    std::shared_ptr<upload_info>
    get_upload_info(const std::string& upload_id) const;
    void append_upload_part_info(const std::string& upload_id, uint16_t part_id,
                                 const dedupe_response& resp,
                                 const std::string& data);
    bool remove_upload(const std::string& upload_id, const std::string& bucket,
                       const std::string& object_key);
    std::map<std::string, std::string>
    list_multipart_uploads(const std::string&) const;

private:
    mutable std::mutex mutex{};

    std::unordered_map<std::string, std::shared_ptr<upload_info>>
        m_upload_infos;

    struct list_state {
        std::unordered_map<std::string, std::vector<std::string>>
            bucket_to_uploads_container;
        std::unordered_map<std::string, std::string> uploads_to_key_container;
    };

    list_state m_list_state;
};

struct state {
    struct upload_state m_uploads;
};

} // namespace uh::cluster
