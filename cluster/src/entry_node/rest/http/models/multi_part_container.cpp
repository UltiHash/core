#include "multi_part_container.h"

#include <utility>

namespace uh::cluster::rest::http::model
{

    multi_part_container::multi_part_container(std::string id) : m_upload_id(std::move(id))
    {}

} // uh::cluster::rest::http::model