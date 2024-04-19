#include "state.h"
#include "common/telemetry/log.h"
#include "common/utils/md5.h"
#include "common/utils/random.h"
#include <algorithm>
#include <source_location>

namespace uh::cluster {

std::map<std::string, std::string>
upload_state::list_multipart_uploads(const std::string& bucket) {

    clear_infos();

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
    clear_infos();

    auto info = std::make_shared<upload_info>();
    info->key = object_key;
    info->bucket = bucket;

    std::string id;

    {
        std::lock_guard<std::mutex> lock(mutex);

        do {
            id = generate_unique_id();
        } while (m_infos.contains(id));

        m_infos.emplace(id, info);
    }

    LOG_DEBUG() << "insert upload, id " << id << ", bucket: " << bucket
                << ", key: " << object_key;

    return id;
}

void upload_state::remove_upload(const std::string& id) {
    LOG_DEBUG() << "remove upload, id: " << id;

    clear_infos();

    std::lock_guard<std::mutex> lock(mutex);
    auto it = m_infos.find(id);
    if (it != m_infos.end() && !it->second->erased) {
        m_deletions.push(info_deletion(it, DEFAULT_TIMEOUT));
        it->second->erased = true;
    }
}

std::shared_ptr<upload_info>
upload_state::get_upload_info(const std::string& id) {
    clear_infos();

    LOG_DEBUG() << "get upload info, id: " << id;

    std::lock_guard<std::mutex> lock(mutex);

    auto itr = m_infos.find(id);
    if (itr != m_infos.end()) {
        return itr->second;
    } else {
        return {};
    }
}

bool upload_state::contains_upload(const std::string& id) {
    clear_infos();

    std::lock_guard<std::mutex> lock(mutex);
    return m_infos.contains(id);
}

void upload_state::append_upload_part_info(const std::string& id, uint16_t part,
                                           const dedupe_response& resp,
                                           std::span<char> data) {

    clear_infos();

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
}

void upload_state::clear_infos() {
    auto now = clock::now();

    std::lock_guard<std::mutex> lock(mutex);
    while (!m_deletions.empty() && now > m_deletions.top().when) {
        m_infos.erase(m_deletions.top().where);
        m_deletions.pop();
    }
}

} // namespace uh::cluster
