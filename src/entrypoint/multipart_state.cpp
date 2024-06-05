#include "multipart_state.h"

#include "common/telemetry/log.h"
#include "common/utils/random.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

std::map<std::string, std::string>
multipart_state::list_multipart_uploads(const std::string& bucket) {

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

std::string multipart_state::insert_upload(std::string bucket,
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

void multipart_state::remove_upload(const std::string& id) {
    LOG_DEBUG() << "remove upload, id: " << id;

    clear_infos();

    std::lock_guard<std::mutex> lock(mutex);
    if (auto it = find(id); !it->second->erased) {
        m_deletions.push(info_deletion(it, DEFAULT_TIMEOUT));
        it->second->erased = true;
    }
}

std::shared_ptr<upload_info>
multipart_state::get_upload_info(const std::string& id) {
    LOG_DEBUG() << "get upload info, id: " << id;

    clear_infos();

    std::lock_guard<std::mutex> lock(mutex);
    return find(id)->second;
}

bool multipart_state::contains_upload(const std::string& id) {
    clear_infos();

    std::lock_guard<std::mutex> lock(mutex);
    return m_infos.contains(id);
}

void multipart_state::append_upload_part_info(const std::string& id,
                                              uint16_t part,
                                              const dedupe_response& resp,
                                              size_t data_size,
                                              std::string&& md5) {

    LOG_DEBUG() << "append upload part info, id: " << id << ", part: " << part;

    clear_infos();

    std::lock_guard<std::mutex> lock(mutex);

    auto& total_resp = find(id)->second;
    total_resp->etags.emplace(part, std::move(md5));
    total_resp->effective_size += resp.effective_size;
    total_resp->data_size += data_size;
    total_resp->part_sizes.emplace(part, data_size);
    total_resp->addresses.emplace(part, resp.addr);
}

void multipart_state::clear_infos() {
    auto now = clock::now();

    std::lock_guard<std::mutex> lock(mutex);
    while (!m_deletions.empty() && now > m_deletions.top().when) {
        m_infos.erase(m_deletions.top().where);
        m_deletions.pop();
    }
}

std::unordered_map<std::string, std::shared_ptr<upload_info>>::iterator
multipart_state::find(const std::string& id) {

    if (id.empty()) {
        throw command_exception(http::status::bad_request, "BadUploadId",
                                "invalid upload id");
    }

    auto itr = m_infos.find(id);
    if (itr == m_infos.end()) {
        throw command_exception(http::status::not_found, "NoSuchUpload",
                                "upload id not found");
    }

    return itr;
}

} // namespace uh::cluster
