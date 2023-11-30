#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include "entry_node/rest/utils/hashing/hash.h"

namespace uh::cluster::rest::utils
{

    struct parts
    {
    public:

        /*
         * THIS STRUCT IS IMMUTABLE ONCE INITIALIZED. IT CAN ONLY BE CONSTRUCTED AND DESTRUCTED
         */
        struct part_data
        {
        private:
            rest::utils::hashing::MD5 md5 {};

        public:
            explicit part_data(std::string&&);

            const std::string body {};
            const std::string etag {};
        };

        std::shared_ptr<part_data> find(std::uint16_t part_num) const;
        bool put_single_part(std::uint16_t part_number, std::string&& body);
        size_t size() const;

        std::map<std::uint16_t, std::pair<const std::string&, std::size_t>> get_parts() const;

        parts() = default;

    private:
        mutable std::mutex mutex {};
        std::map<std::uint16_t, std::shared_ptr<part_data>> parts_container;
    };

    struct upload_state
    {
        upload_state() = default;

        bool insert_upload(std::string upload_id);
        std::shared_ptr<parts> get_parts_container(const std::string& upload_id) const;
        bool remove_upload(const std::string& upload_id);

    private:
        mutable std::mutex mutex {};
        /* { upload_id, shared ptr to parts struct } */
        std::unordered_map<std::string, std::shared_ptr<parts>> upload_to_parts_container;
    };

    struct server_state
    {
        struct upload_state m_uploads;
    };

} // uh::cluster::rest::utils
