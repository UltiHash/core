#include "server_state.h"
#include <algorithm>

namespace uh::cluster::rest::utils {

std::map<std::string, std::string>
upload_state::list_multipart_uploads(const std::string& bucket) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::map<std::string, std::string> multipart_uploads{};
    auto bucket_iter = m_list_state.bucket_to_uploads_container.find(bucket);
    if (bucket_iter != m_list_state.bucket_to_uploads_container.end()) {
        for (const auto& value : bucket_iter->second) {
            multipart_uploads.emplace(
                value, m_list_state.uploads_to_key_container.at(value));
        }
    }

    return multipart_uploads;
}

bool upload_state::insert_upload(std::string upload_id, std::string bucket,
                                 std::string object_key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto pair =
        m_upload_infos.emplace(upload_id, std::make_shared<upload_info>());

    // extra information for list multipart uploads
    if (m_list_state.bucket_to_uploads_container.find(bucket) ==
        m_list_state.bucket_to_uploads_container.end()) {
        m_list_state.bucket_to_uploads_container.emplace(
            std::move(bucket), std::vector<std::string>{upload_id});
    } else {
        m_list_state.bucket_to_uploads_container[bucket].emplace_back(
            upload_id);
    }

    m_list_state.uploads_to_key_container.emplace(std::move(upload_id),
                                                  std::move(object_key));

    return pair.second;
}

bool upload_state::remove_upload(const std::string& upload_id,
                                 const std::string& bucket,
                                 const std::string& object_key) {
    std::lock_guard<std::mutex> lock(mutex);

    bool status = m_upload_infos.erase(upload_id);

    auto bucket_iter = m_list_state.bucket_to_uploads_container.find(bucket);
    if (bucket_iter != m_list_state.bucket_to_uploads_container.end()) {
        auto object_key_itr = std::find(bucket_iter->second.begin(),
                                        bucket_iter->second.end(), object_key);
        if (object_key_itr != bucket_iter->second.end()) {
            bucket_iter->second.erase(object_key_itr);

            if (bucket_iter->second.empty()) {
                m_list_state.bucket_to_uploads_container.erase(bucket);
            }
        }
    }
    m_list_state.uploads_to_key_container.erase(upload_id);

    return status;
}

std::shared_ptr<upload_info>
upload_state::get_upload_info(const std::string& upload_id) const {
    std::lock_guard<std::mutex> lock(
        mutex); // use shared lock here since we do not modify it
    auto itr = m_upload_infos.find(upload_id);
    if (itr != m_upload_infos.end()) {
        return itr->second;
    } else {
        return {};
    }
}

bool upload_state::contains_upload(const std::string& upload_id) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto itr = m_upload_infos.find(upload_id);
    if (itr != m_upload_infos.end()) {
        return true;
    } else {
        return false;
    }
}

void upload_state::append_upload_part_info(const std::string& upload_id,
                                           uint16_t part_id,
                                           const dedupe_response& resp,
                                           const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex);
    auto& total_resp = m_upload_infos[upload_id];
    if (!data.empty()) {
        rest::utils::hashing::MD5 md5_calculator;
        total_resp->etags.emplace(part_id, md5_calculator.calculateMD5(data));
    } else { // default etag
        total_resp->etags.emplace(
            part_id,
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }
    total_resp->effective_size += resp.effective_size;
    total_resp->data_size += data.size();
    total_resp->part_sizes.emplace(part_id, data.size());
    total_resp->addresses.emplace(part_id, resp.addr);
    if (total_resp->upload_init_time == 0) {
        const auto time = std::chrono::steady_clock::now();
        total_resp->upload_init_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                time.time_since_epoch())
                .count();
    }
}

} // namespace uh::cluster::rest::utils
