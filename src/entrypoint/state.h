#ifndef ENTRYPOINT_STATE_H
#define ENTRYPOINT_STATE_H

#include "common/types/common_types.h"
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

namespace uh::cluster {

struct upload_info {
    size_t effective_size{0};
    size_t data_size{0};
    std::map<uint16_t, std::string> etags;
    std::map<uint16_t, size_t> part_sizes;
    std::map<uint16_t, address> addresses;

    std::string key;
    std::string bucket;
    bool erased = false;

    [[nodiscard]] address generate_total_address() const {
        address addr;
        for (const auto& a : addresses) {
            addr.append_address(a.second);
        }
        return addr;
    }
};

struct upload_state {
    using clock = std::chrono::system_clock;
    using duration = std::chrono::seconds;
    using time_point = std::chrono::time_point<clock>;

    static constexpr auto DEFAULT_TIMEOUT = duration(300);

    std::string insert_upload(std::string bucket, std::string object_key);
    bool contains_upload(const std::string& id);
    std::shared_ptr<upload_info> get_upload_info(const std::string& id);
    void append_upload_part_info(const std::string& id, uint16_t part_id,
                                 const dedupe_response& resp,
                                 size_t data_size, std::string&& md5);

    void remove_upload(const std::string& id);

    std::map<std::string, std::string>
    list_multipart_uploads(const std::string&);

private:
    void clear_infos();

    std::unordered_map<std::string, std::shared_ptr<upload_info>>::iterator
    find(const std::string& id);

    mutable std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<upload_info>> m_infos;

    struct info_deletion {
    public:
        info_deletion(
            std::unordered_map<std::string,
                               std::shared_ptr<upload_info>>::iterator where,
            duration timeout)
            : where(where),
              when(clock::now() + timeout) {}

        bool operator>(const info_deletion& other) const {
            return when > other.when;
        }

        std::unordered_map<std::string, std::shared_ptr<upload_info>>::iterator
            where;
        time_point when;
    };
    std::priority_queue<info_deletion, std::vector<info_deletion>,
                        std::greater<info_deletion>>
        m_deletions;
};

struct state {
    struct upload_state m_uploads;
};

} // namespace uh::cluster

#endif
