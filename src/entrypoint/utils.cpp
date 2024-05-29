#include "utils.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

void reference_collection::check_storage_size(std::size_t increment) const {
    auto new_size = data_storage_size + increment;
    if (new_size > max_data_size) {
        throw error_exception(error::storage_limit_exceeded);
    }

    static unsigned warn_counter = 0;
    if (new_size * 100 > max_data_size * SIZE_LIMIT_WARNING_PERCENTAGE) {
        if (warn_counter == 0) {
            LOG_WARN() << "over " << SIZE_LIMIT_WARNING_PERCENTAGE
                       << "% of storage limit reached";
            warn_counter = SIZE_LIMIT_WARNING_INTERVAL;
        }

        --warn_counter;
    }

    data_storage_size = new_size;
}

void reference_collection::free_storage_size(std::size_t decrement) const {
    std::size_t current = data_storage_size;
    std::size_t desired;

    do {
        desired = current < decrement ? 0ull : current - decrement;
    } while (!data_storage_size.compare_exchange_weak(current, desired));
}

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

    if (!bucket_id.empty()) {
        if (bucket_id.size() < 3 || bucket_id.size() > 63) {
            throw command_exception(http::status::bad_request,
                                    "InvalidBucketName",
                                    "bucket name has invalid length");
        }

        std::regex bucket_pattern(
            R"(^(?!(xn--|sthree-|sthree-configurator-))(?!.*-s3alias$)(?!.*--ol-s3$)(?!^(\d{1,3}\.){3}\d{1,3}$)[a-z0-9](?!.*\.\.)(?!.*[.\s-][.\s-])[a-z0-9.-]*[a-z0-9]$)");
        if (!std::regex_match(bucket_id, bucket_pattern)) {
            throw command_exception(http::status::bad_request,
                                    "InvalidBucketName",
                                    "bucket name has invalid characters");
        }
    }

    return std::make_tuple(bucket_id, object_key);
}

} // namespace uh::cluster
