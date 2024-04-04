#include "utils.h"
#include "common/utils/worker_pool.h"

namespace uh::cluster {

coro<dedupe_response>
integration::integrate_data(std::span<const char> data,
                            const reference_collection& collection) {

    auto dedupe_services = collection.dedupe_services.get_clients();
    if (dedupe_services.empty()) {
        throw std::runtime_error("no deduplicator services available");
    }

    auto dedupe_services_size = dedupe_services.size();
    const auto part_size = static_cast<size_t>(
        std::ceil(static_cast<double>(data.size()) /
                  static_cast<double>(dedupe_services_size)));

    std::vector<dedupe_response> responses(dedupe_services_size);

    auto func = [&](client::acquired_messenger m, long i) -> coro<void> {
        auto chunk = data.subspan(i * part_size, part_size);
        m.get().register_write_buffer(chunk);

        co_await m.get().send_buffers(DEDUPLICATOR_REQ);
        const auto h_dedup = co_await m.get().recv_header();
        responses[i] = co_await m.get().recv_dedupe_response(h_dedup);
    };

    co_await collection.workers.broadcast_from_io_thread_in_io_threads(
        dedupe_services, func);

    dedupe_response resp{.effective_size = 0};

    for (const auto& r : responses) {
        resp.append(r);
    }

    co_return resp;
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

} // namespace uh::cluster
