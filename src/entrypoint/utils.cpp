#include "utils.h"

#include <common/utils/strings.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/formats.h>

using namespace uh::cluster::ep::http;
using uh::cluster::ep::object;

namespace uh::cluster {

namespace {

std::optional<std::string> ident(std::optional<std::string> s) noexcept { return s; }

std::optional<std::string> opt_url_encode(std::optional<std::string> s) noexcept {
    return s ? url_encode(*s) : s;
}

};

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

encoder_function encoder(std::optional<std::string> encoding_type) {
    if (!encoding_type) {
        return ident;
    }

    if (*encoding_type != "url") {
        throw command_exception(status::bad_request, "InvalidArgument",
                                "Encountered unexpected query parameter.");
    }

    return opt_url_encode;
}

void set_default_headers(response& res, const object& obj) {
    res.set("ETag", obj.etag);
    res.set("Content-Type", obj.mime);
    res.set("Last-Modified", imf_fixdate(obj.last_modified));
    res.set("X-Amz-Version-Id", obj.version);
}

} // namespace uh::cluster
