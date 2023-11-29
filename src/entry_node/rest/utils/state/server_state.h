#pragma once

#include <memory>
#include <map>
#include <unordered_map>
#include "common/common_types.h"

namespace uh::cluster::rest::utils
{

    struct parts
    {
    public:
        struct part_data
        {
        public:
            explicit part_data(std::string&&);
            const std::string& get_body();
            const std::string& get_etag();
        private:
            std::mutex mutex {};
            std::string body {};
            std::string etag {};
            address address {};
            std::string effective_size {};
        };

        std::shared_ptr<part_data> find(std::uint16_t part_num);
        bool put_single_part(std::uint16_t part_number, std::string&& body);
        size_t size();

        [[nodiscard]] std::map<std::uint16_t, std::shared_ptr<part_data>>::const_iterator begin() const;
        [[nodiscard]] std::map<std::uint16_t, std::shared_ptr<part_data>>::const_iterator end() const;

        parts() = default;

    private:
        std::mutex mutex {};
        std::map<std::uint16_t, std::shared_ptr<part_data>> parts_container;

    };

    struct upload_state
    {
        upload_state() = default;

        bool insert_upload(std::string upload_id);
        std::shared_ptr<parts> get_parts_container(const std::string& upload_id);
        bool remove_upload(const std::string& upload_id);

    private:
        std::mutex mutex {};
        /* { upload_id, shared ptr to parts struct } */
        std::unordered_map<std::string, std::shared_ptr<parts>> upload_to_parts_container;
    };

    struct server_state
    {
        struct upload_state m_uploads;
    };

} // uh::cluster::rest::utils
