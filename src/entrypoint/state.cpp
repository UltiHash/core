#include "state.h"
#include "common/telemetry/log.h"
#include "common/utils/md5.h"
#include "common/utils/random.h"
#include <algorithm>
#include <source_location>

namespace uh::cluster {

std::map<std::string, std::string>
upload_state::list_multipart_uploads(const std::string& bucket) const {
    LOG_DEBUG() << "list multipart uploads for bucket " << bucket;

    std::lock_guard<std::mutex> lock(mutex);

    std::map<std::string, std::string> rv;
    for (const auto& info : m_infos) {
        if (info.second->bucket == bucket) {
            rv[info.first] = info.second->key;
        }
    }

    return rv;
}

std::string upload_state::insert_upload(std::string bucket,
                                        std::string object_key) {
    auto info = std::make_shared<upload_info>();
    info->key = object_key;
    info->bucket = bucket;

    std::string id;

    {
        std::lock_guard<std::mutex> lock(mutex);

        do {
            id = generate_unique_id();
        } while (!m_infos.contains(id));

        m_infos.emplace(id, info);
    }

    LOG_DEBUG() << "insert upload, id " << id << ", bucket: " << bucket
                << ", key: " << object_key;

    return id;
}

void upload_state::remove_upload(const std::string& id) {

    LOG_DEBUG() << "remove upload, id: " << id;

    std::lock_guard<std::mutex> lock(mutex);
    m_infos.erase(id);
}

std::shared_ptr<upload_info>
upload_state::get_upload_info(const std::string& id) const {

    LOG_DEBUG() << "get upload info, id: " << id;

    std::lock_guard<std::mutex> lock(mutex);

    auto itr = m_infos.find(id);
    if (itr != m_infos.end()) {
        return itr->second;
    } else {
        return {};
    }
}

bool upload_state::contains_upload(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex);
    return m_infos.contains(id);
}

void upload_state::append_upload_part_info(const std::string& id, uint16_t part,
                                           const dedupe_response& resp,
                                           const std::string& data) {

    LOG_DEBUG() << "append upload part info, id: " << id << ", part: " << part;

    std::lock_guard<std::mutex> lock(mutex);
    auto info = m_infos.find(id);
    if (info == m_infos.end()) {
        throw std::runtime_error("unkown upload id: " + id);
    }

    auto& total_resp = info->second;
    if (!data.empty()) {
        total_resp->etags.emplace(part, calculate_md5(data));
    } else { // default etag
        total_resp->etags.emplace(
            part,
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }
    total_resp->effective_size += resp.effective_size;
    total_resp->data_size += data.size();
    total_resp->part_sizes.emplace(part, data.size());
    total_resp->addresses.emplace(part, resp.addr);
    if (total_resp->upload_init_time == 0) {
        const auto time = std::chrono::steady_clock::now();
        total_resp->upload_init_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                time.time_since_epoch())
                .count();
    }
}

} // namespace uh::cluster
