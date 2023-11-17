#pragma once

#include <memory>
#include <vector>
#include "ts_unordered_map.h"
#include "ts_map.h"
#include "ts_vector.h"

namespace uh::cluster::rest::utils
{

    class state
    {
    public:
        state() = default;
        ~state() = default;

        std::string generate_and_add_upload_id(const std::string&, const std::string&);

        ts_unordered_map<std::string, std::shared_ptr<ts_map<uint16_t, std::pair<std::string, std::string>>>>&
        get_multipart_container();

    private:
        /* { upload_id, { part_number, < etag, body > } } } */
        ts_unordered_map<std::string, std::shared_ptr<ts_map<uint16_t, std::pair<std::string, std::string>>>> m_parts_container;
        /* { bucket_name, { keys, [ upload_id, ... ] } } */
//        ts_unordered_map<std::string, std::shared_ptr<ts_map<std::string, ts_vector<std::string>>>> m_current_multiparts;
    };

} // uh::cluster::rest::utils
