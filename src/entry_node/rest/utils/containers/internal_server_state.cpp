#include "internal_server_state.h"
#include "entry_node/rest/utils/generator/generator.h"
#include <common/log.h>

namespace uh::cluster::rest::utils
{
    std::string state::generate_and_add_upload_id(const std::string& bucket_name, const std::string& object_key)
    {
        std::string upload_id = generator::generate_unique_id();

        m_parts_container.emplace(upload_id,
                                   std::make_shared<ts_map<uint16_t, std::pair<std::string, std::string>>>());


//        auto find_key_ptr = m_current_multiparts.find(bucket_name);
//        if (find_key_ptr != m_current_multiparts.end())
//        {
//            if (find_key_ptr->second->find(upload_id) != find_key_ptr->second->end())
//            {
//
//            }
//        }
//        else
//        {
//
//        }

        return upload_id;
    }

    ts_unordered_map<std::string, std::shared_ptr<ts_map<uint16_t, std::pair<std::string, std::string>>>>&
    state::get_multipart_container()
    {
        return m_parts_container;
    }

} // uh::cluster::rest::utils
