#include "utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

std::vector<collapsed_objects>
retrieval::collapse(const std::vector<object>& objects,
                    std::optional<std::string> delimiter,
                    std::optional<std::string> prefix) {
    std::vector<collapsed_objects> collapsed_objs;

    for (std::string previous_prefix; const auto& object : objects) {
        size_t delimiter_index = std::string::npos;

        if (delimiter) {
            if (prefix) {
                delimiter_index = object.name.find(*delimiter, prefix->size());
            } else {
                delimiter_index = object.name.find(*delimiter);
            }
        }

        if (delimiter_index != std::string::npos) {
            auto delimiter_prefix = object.name.substr(0, delimiter_index + 1);
            if (previous_prefix != delimiter_prefix) {
                collapsed_objs.emplace_back(delimiter_prefix, std::nullopt);
                previous_prefix = delimiter_prefix;
            }
        } else {
            collapsed_objs.emplace_back(std::nullopt, std::cref(object));
        }
    }

    return collapsed_objs;
}

std::tuple<std::string, std::string>
extract_bucket_and_object(boost::urls::url url) {
    std::string bucket_id;
    std::string object_key;

    for (const auto& seg : url.segments()) {
        if (bucket_id.empty())
            bucket_id = seg;
        else
            object_key += seg + '/';
    }

    if (!object_key.empty())
        object_key.pop_back();

    return std::make_tuple(bucket_id, object_key);
}

} // namespace uh::cluster
