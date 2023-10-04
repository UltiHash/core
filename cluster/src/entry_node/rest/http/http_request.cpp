#include "entry_node/rest/utils/string/string_utils.h"
#include "http_request.h"

namespace uh::cluster::rest::http
{

    [[nodiscard]] std::map<std::string, std::string> http_request::get_headers() const
    {
        std::map<std::string, std::string> headers_map;

        for (const auto& pair : m_req.get() )
        {
            headers_map[rest::utils::string_utils::to_lower( std::string { pair.name_string().data(), pair.name_string().length() }.c_str() )] = pair.value();
        }

        return headers_map;
    }

} // namespace uh::cluster::rest::http
