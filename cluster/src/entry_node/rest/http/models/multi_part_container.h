#pragma once

#include <entry_node/rest/http/http_request.h>
#include <entry_node/rest/utils/containers/ts_map.h>

namespace uh::cluster::rest::http::model
{

    class multi_part_container
    {
    public:
        explicit multi_part_container(std::string id);
        ~multi_part_container() = default;

    private:
        std::string m_upload_id;
        utils::ts_map<uint16_t, std::string> m_parts_container;
    };

} // uh::cluster::rest::http::model
