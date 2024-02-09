#pragma once

#include "boost/asio.hpp"
#include "common/registry/services.h"
#include "entrypoint/rest/utils/state/server_state.h"

namespace uh::cluster::entry {

    typedef struct {
        boost::asio::io_context& ioc;
        boost::asio::thread_pool& workers;
        const services<DEDUPLICATOR_SERVICE>& dedup_services;
        const services<DIRECTORY_SERVICE>& directory_services;
        rest::utils::server_state& server_state;
    } entrypoint_state;

    struct for_some_reason{
    public:
        static coro <dedupe_response> integrate_data (const std::list <std::string_view>& data_pieces,
                                                      boost::asio::io_context& ioc,
                                                      boost::asio::thread_pool& workers,
                                                      const services<DEDUPLICATOR_SERVICE>& dedupe_services) {

            size_t total_size = 0;
            std::map <size_t, std::string_view> offset_pieces;
            for (const auto& dp: data_pieces) {
                offset_pieces.emplace_hint(offset_pieces.cend(), total_size, dp);
                total_size += dp.size();
            }

            auto dedup_services = dedupe_services.get_clients();
            auto dedup_services_size = dedup_services.size();
            const auto part_size = static_cast <size_t> (std::ceil (static_cast <double> (total_size) / static_cast <double> (dedup_services_size)));

            std::vector<dedupe_response> responses(dedup_services_size);

            auto func = [] (size_t part_size,
                            const std::map <size_t, std::string_view>& offset_pieces,
                            std::vector <dedupe_response>& responses,
                            client::acquired_messenger m,
                            long i) -> coro <void> {
                const auto my_offset = i * part_size;
                std::list <std::string_view> my_pieces;
                auto offset_itr = offset_pieces.upper_bound(my_offset);
                offset_itr --;
                size_t my_data_size = 0;
                auto seek = my_offset - offset_itr->first;
                while (my_data_size < part_size) {
                    const auto piece_size = offset_itr->second.size();
                    const auto piece_size_for_me = std::min (piece_size, part_size - my_data_size);
                    my_pieces.emplace_back (offset_itr->second.substr(seek, piece_size_for_me));
                    seek = 0;
                    m.get().register_write_buffer(my_pieces.back());
                    offset_itr ++;
                    my_data_size += piece_size_for_me;
                    if (offset_itr == offset_pieces.cend()) {
                        break;
                    }
                }

                co_await m.get().send_buffers(DEDUPE_REQ);
                const auto h_dedup = co_await m.get().recv_header();
                responses [i] = co_await m.get().recv_dedupe_response(h_dedup);
            };

            co_await worker_utils::broadcast_from_io_thread_in_io_threads (dedup_services,
                                                                           ioc,
                                                                           workers,
                                                                           std::bind_front (func, part_size, std::cref (offset_pieces), std::ref (responses)));


            dedupe_response resp {.effective_size = 0};

            for (const auto& r: responses) {
                resp.effective_size += r.effective_size;
                resp.addr.append_address(r.addr);
            }
            co_return resp;
        }
    };

} // uh::cluster::entry
