#include "utils.h"
#include "common/utils/worker_pool.h"

namespace uh::cluster {

coro<dedupe_response>
integration::integrate_data(const std::list<std::string_view>& data_pieces,
                            const reference_collection& collection) {

    size_t total_size = 0;
    std::map<size_t, std::string_view> offset_pieces;
    for (const auto& dp : data_pieces) {
        offset_pieces.emplace_hint(offset_pieces.cend(), total_size, dp);
        total_size += dp.size();
    }

    auto dedupe_services = collection.dedupe_services.get_clients();
    if (dedupe_services.empty()) {
        throw std::runtime_error("no deduplicator services available");
    }

    auto dedupe_services_size = dedupe_services.size();
    const auto part_size = static_cast<size_t>(
        std::ceil(static_cast<double>(total_size) /
                  static_cast<double>(dedupe_services_size)));

    std::vector<dedupe_response> responses(dedupe_services_size);

    auto func = [](size_t part_size,
                   const std::map<size_t, std::string_view>& offset_pieces,
                   std::vector<dedupe_response>& responses,
                   client::acquired_messenger m, long i) -> coro<void> {
        const auto my_offset = i * part_size;
        std::list<std::string_view> my_pieces;
        auto offset_itr = offset_pieces.upper_bound(my_offset);
        offset_itr--;
        size_t my_data_size = 0;
        auto seek = my_offset - offset_itr->first;
        while (my_data_size < part_size) {
            const auto piece_size = offset_itr->second.size();
            const auto piece_size_for_me =
                std::min(piece_size, part_size - my_data_size);
            my_pieces.emplace_back(
                offset_itr->second.substr(seek, piece_size_for_me));
            seek = 0;
            m.get().register_write_buffer(my_pieces.back());
            offset_itr++;
            my_data_size += piece_size_for_me;
            if (offset_itr == offset_pieces.cend()) {
                break;
            }
        }

        co_await m.get().send_buffers(DEDUPLICATOR_REQ);
        const auto h_dedup = co_await m.get().recv_header();
        responses[i] = co_await m.get().recv_dedupe_response(h_dedup);
    };

    co_await collection.workers.broadcast_from_io_thread_in_io_threads(
        dedupe_services,
        std::bind_front(func, part_size, std::cref(offset_pieces),
                        std::ref(responses)));

    dedupe_response resp{.effective_size = 0};

    for (const auto& r : responses) {
        resp.effective_size += r.effective_size;
        resp.addr.append_address(r.addr);
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
