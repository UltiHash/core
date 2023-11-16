#include "internal_server_state.h"
#include "entry_node/rest/utils/generator/generator.h"
#include <common/log.h>

namespace uh::cluster::rest::utils
{
    std::string state::generate_and_add_upload_id(const std::string& bucket_name)
    {
        std::string upload_id = generator::generate_unique_id();

        m_parts_container.emplace(bucket_name+upload_id,
                                   std::make_shared<ts_map<uint16_t, std::pair<std::string, std::string>>>());

        return upload_id;
    }

} // uh::cluster::rest::utils
