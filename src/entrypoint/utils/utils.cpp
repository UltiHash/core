#include "utils.h"
#include "common/utils/worker_utils.h"

namespace uh::cluster {

coro<dedupe_response>
integration::integrate_data(const std::list<std::string_view>& data_pieces,
                            const entrypoint_state& state) {

    size_t total_size = 0;
    std::map<size_t, std::string_view> offset_pieces;
    for (const auto& dp : data_pieces) {
        offset_pieces.emplace_hint(offset_pieces.cend(), total_size, dp);
        total_size += dp.size();
    }

    auto dedupe_services = state.dedupe_services.get_clients();
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

        co_await m.get().send_buffers(DEDUPE_REQ);
        const auto h_dedup = co_await m.get().recv_header();
        responses[i] = co_await m.get().recv_dedupe_response(h_dedup);
    };

    co_await worker_utils::broadcast_from_io_thread_in_io_threads(
        dedupe_services, state.ioc, state.workers,
        std::bind_front(func, part_size, std::cref(offset_pieces),
                        std::ref(responses)));

    dedupe_response resp{.effective_size = 0};

    for (const auto& r : responses) {
        resp.effective_size += r.effective_size;
        resp.addr.append_address(r.addr);
    }
    co_return resp;
}

const char* command_unknown_exception::what() const noexcept {
    return "command not found";
}

} // namespace uh::cluster
