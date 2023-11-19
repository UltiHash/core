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


        // for listing multi-parts uploads
        auto itr = m_current_bucket_multiparts.find(bucket_name);
        if (itr != m_current_bucket_multiparts.end())
        {
            auto obj_ptr = itr->second->find(object_key);
            if (obj_ptr != itr->second->end())
            {
                obj_ptr->second->push_back(upload_id);
            }
            else
            {
                itr->second->emplace(object_key, std::make_shared<ts_vector<std::string>>(upload_id));
            }
        }
        else
        {
            m_current_bucket_multiparts.emplace(bucket_name,
                                           std::make_shared<ts_map<std::string, std::shared_ptr<ts_vector<std::string>>>>
                                           (object_key));
            m_current_bucket_multiparts.find(bucket_name)->second->find(object_key)->second = std::make_shared<ts_vector<std::string>>(upload_id);
        }

        return upload_id;
    }

    ts_unordered_map<std::string, std::shared_ptr<ts_map<uint16_t, std::pair<std::string, std::string>>>>&
    state::get_multipart_container()
    {
        return m_parts_container;
    }

    ts_unordered_map<std::string, std::shared_ptr<ts_map<std::string, std::shared_ptr<ts_vector<std::string>>>>>&
    state::get_bucket_multiparts()
    {
        return m_current_bucket_multiparts;
    }

} // uh::cluster::rest::utils
