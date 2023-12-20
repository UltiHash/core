#include "server_state.h"
#include <algorithm>

namespace uh::cluster::rest::utils
{

    std::shared_ptr<parts::part_data> parts::find(std::uint16_t part_num) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto itr = parts_container.find(part_num);
        if (itr != parts_container.end())
        {
            return itr->second;
        }
        else
        {
            return {};
        }
    }


    parts::part_data::part_data(std::string&& recv_body) : body(std::move(recv_body)), etag(std::move(md5.calculateMD5(body)))
    {}


    bool parts::put_single_part(std::uint16_t part_number, std::string&& body)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return parts_container.emplace(part_number, std::make_shared<part_data>(std::move(body))).second;
    }


    size_t parts::size() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return parts_container.size();
    }


    std::map<std::uint16_t, std::pair<const std::string&, std::size_t>> parts::get_parts() const
    {
        std::lock_guard<std::mutex> lock(mutex);

        std::map<std::uint16_t, std::pair<const std::string&, std::size_t>> parts_and_sizes;
        for (const auto& pair : parts_container)
        {
            parts_and_sizes.emplace(pair.first, std::pair<const std::string&, std::size_t>{pair.second->body, pair.second->body.size()});
        }

        return parts_and_sizes;
    }

    std::map<std::string, std::string> upload_state::list_multipart_uploads(const std::string& bucket) const
    {
        std::lock_guard<std::mutex> lock(mutex);

        std::map<std::string, std::string> multipart_uploads {};
        auto bucket_iter = m_list_state.bucket_to_uploads_container.find(bucket);
        if (bucket_iter != m_list_state.bucket_to_uploads_container.end())
        {
            for (const auto& value : bucket_iter->second)
            {
                multipart_uploads.emplace(value, m_list_state.uploads_to_key_container.at(value));
            }
        }

        return multipart_uploads;
    }


    bool upload_state::insert_upload(std::string upload_id, std::string bucket, std::string object_key)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto pair = upload_to_parts_container.emplace(upload_id, std::make_shared<parts>());

        // extra information for list multipart uploads
        if (m_list_state.bucket_to_uploads_container.find(bucket) == m_list_state.bucket_to_uploads_container.end())
        {
            m_list_state.bucket_to_uploads_container.emplace(std::move(bucket), std::vector<std::string>{upload_id});
        }
        else
        {
            m_list_state.bucket_to_uploads_container[bucket].emplace_back(upload_id);
        }

        m_list_state.uploads_to_key_container.emplace(std::move(upload_id), std::move(object_key));

        return pair.second;
    }


    bool upload_state::remove_upload(const std::string& upload_id, const std::string& bucket, const std::string& object_key)
    {
        std::lock_guard<std::mutex> lock(mutex);

        bool status = upload_to_parts_container.erase(upload_id);

        auto bucket_iter = m_list_state.bucket_to_uploads_container.find(bucket);
        if (bucket_iter != m_list_state.bucket_to_uploads_container.end())
        {
            auto object_key_itr = std::find(bucket_iter->second.begin(), bucket_iter->second.end(), object_key);
            if (object_key_itr != bucket_iter->second.end())
            {
                bucket_iter->second.erase(object_key_itr);

                if (bucket_iter->second.empty())
                {
                    m_list_state.bucket_to_uploads_container.erase(bucket);
                }
            }
        }
        m_list_state.uploads_to_key_container.erase(upload_id);

        return status;
    }


    std::shared_ptr<parts> upload_state::get_parts_container(const std::string& upload_id) const
    {
        std::lock_guard<std::mutex> lock(mutex); // use shared lock here since we do not modify it
        auto itr = upload_to_parts_container.find(upload_id);
        if (itr != upload_to_parts_container.end())
        {
            return itr->second;
        }
        else
        {
            return {};
        }
    }


    bool upload_state::contains_upload(const std::string& upload_id) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto itr = upload_to_parts_container.find(upload_id);
        if (itr != upload_to_parts_container.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

} // uh::cluster::rest::utils
