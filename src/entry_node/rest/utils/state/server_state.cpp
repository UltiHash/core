#include "server_state.h"
#include "entry_node/rest/utils/hashing/hash.h"

namespace uh::cluster::rest::utils
{

    std::shared_ptr<parts::part_data> parts::find(std::uint16_t part_num)
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

    parts::part_data::part_data(std::string&& recv_body)
    {
        body = std::move(recv_body);
        rest::utils::hashing::MD5 md5;
        etag = std::move(md5.calculateMD5(body));
    }


    bool parts::put_single_part(std::uint16_t part_number, std::string&& body)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return parts_container.emplace(part_number, std::make_shared<part_data>(std::move(body))).second;
    }


    bool upload_state::insert_upload(std::string upload_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto pair = upload_to_parts_container.emplace(std::move(upload_id), std::make_shared<parts>());
        return pair.second;
    }


    bool upload_state::remove_upload(const std::string& upload_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return upload_to_parts_container.erase(upload_id);
    }


    std::shared_ptr<parts> upload_state::get_parts_container(const std::string& upload_id)
    {
        std::lock_guard<std::mutex> lock(mutex);
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


} // uh::cluster::rest::utils