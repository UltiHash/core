#pragma once

#include <memory>
#include <map>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "entrypoint/rest/utils/hashing/hash.h"
#include "common/utils/common_types.h"
#include <chrono>
#include <list>

namespace uh::cluster::rest::utils
{
    struct upload_info {
        address m_addr;
        size_t effective_size{0};
        size_t data_size{0};
        std::list <std::string> etags;
        unsigned long long upload_init_time{0};
    };

    struct upload_state
    {
        upload_state() = default;

        bool insert_upload(std::string upload_id, std::string bucket, std::string object_key);
        bool contains_upload(const std::string& bucket) const;
        std::shared_ptr<upload_info> get_upload_info (const std::string& upload_id) const;
        void append_upload_part_info (const std::string& upload_id, const dedupe_response& resp, const std::string& data);
        bool remove_upload(const std::string& upload_id, const std::string& bucket, const std::string& object_key);
        std::map<std::string, std::string> list_multipart_uploads(const std::string&) const;

    private:
        mutable std::mutex mutex {};

        rest::utils::hashing::MD5 m_md5_calculator;
        std::unordered_map<std::string, std::shared_ptr<upload_info>> m_upload_infos;

        struct list_state
        {
            std::unordered_map<std::string, std::vector<std::string>> bucket_to_uploads_container;
            std::unordered_map<std::string, std::string> uploads_to_key_container;
        };

        list_state m_list_state;
    };

    struct server_state
    {
        struct upload_state m_uploads;
    };

} // uh::cluster::rest::utils
