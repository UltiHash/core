//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_REST_HANDLER_H
#define CORE_ENTRY_NODE_REST_HANDLER_H

#include "common/protocol_handler.h"
#include "s3_parser.h"
#include "network/client.h"
#include <iostream>

namespace uh::cluster {

class entry_node_rest_handler {
public:

    entry_node_rest_handler (std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes):
        m_dedupe_nodes (dedupe_nodes),
        m_directory_nodes (directory_nodes)
    {}

    coro < http::response<http::string_body> > handle (const parsed_request_wrapper& req) {

        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::server, "UltiHash v0.2.0");
        res.set(http::field::content_type, "text/html");

        const auto size_mb = static_cast <double> (req.body.size()) / static_cast <double> (1024ul * 1024ul);

        std::chrono::time_point <std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now ();

        std::stringstream metrics;

        if (req.req_type == s3_req_type::put_object) {
            auto m_dedup = m_dedupe_nodes.at(get_round_robin_index(m_dedupe_node_index, m_dedupe_nodes.size())).acquire_messenger();
            co_await m_dedup.get().send (DEDUPE_REQ, req.body);
            const auto h_dedup = co_await m_dedup.get().recv_header();
            const auto resp = co_await m_dedup.get().recv_dedupe_response(h_dedup);

            auto effective_size = static_cast <double> (resp.second.effective_size) / static_cast <double> (1024ul * 1024ul);
            std::cout << "effective size " << effective_size << " MB" << std::endl;
            std::cout << "original size " << size_mb << " MB" << std::endl;
            auto space_saving = 1.0 - static_cast <double> (resp.second.effective_size) / static_cast <double> (req.body.size());
            std::cout << "space saving " << space_saving << std::endl;

            metrics << "effective size: " << effective_size << " MB, original size: " << size_mb << " MB, space savings: " << space_saving;
            // TODO send the address resp.second.addr to the directory
            //co_await m.get().send_rest_request(DIR_PUT_OBJ_REQ, req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
            directory_request dir_req;
            dir_req.bucket_id = req.bucket_id;
            dir_req.object_key = req.object_key;
            dir_req.addr = resp.second.addr;
            co_await m_dir.get().send_directory_request(DIR_PUT_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();
            if(h_dir.type == FAILURE) {
                throw std::runtime_error("Failed to add the fragment address of object " + dir_req.bucket_id + "/" + dir_req.object_key + " to the directory.");
                //TODO: consider using custom exceptions to indicate if and how the error gets communicated to the HTTP client.
            }
        }
        else {
            // TODO send the request to the directory

        }

        const auto stop = std::chrono::steady_clock::now ();
        const std::chrono::duration <double> duration = stop - start;
        std::cout << "duration " << duration.count() << " s" << std::endl;
        const auto bandwidth = size_mb / duration.count();
        std::cout << "bandwidth " << bandwidth << " MB/s" << std::endl;

        metrics << "bandwidth " << bandwidth << " MB/s";

        res.body() = metrics.str();
        res.prepare_payload();

        co_return res;
    }

private:

    static size_t get_round_robin_index (std::atomic <size_t>& current_index, const size_t total_size) {
        auto index = current_index.load();
        auto new_val = (index + 1) % total_size;
        while (!current_index.compare_exchange_weak (index, new_val)) {
            index = current_index.load();
            new_val = (index + 1) % total_size;
        }
        return index;
    }

    std::atomic <size_t> m_dedupe_node_index {};
    std::atomic <size_t> m_directory_node_index {};

    std::vector <client>& m_dedupe_nodes;
    std::vector <client>& m_directory_nodes;
};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_REST_HANDLER_H
