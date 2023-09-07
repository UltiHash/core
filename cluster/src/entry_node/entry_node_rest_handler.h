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

    coro <void> handle (const parsed_request_wrapper& req) {
        if (req.req_type == s3_req_type::put_object) {
            auto m = m_dedupe_nodes.at(get_round_robin_index(m_dedupe_node_index, m_dedupe_nodes.size())).acquire_messenger();
            co_await m.get().send (DEDUPE_REQ, req.body);
            const auto h = co_await m.get().recv_header();
            const auto resp = co_await m.get().recv_dedupe_response(h);
            std::cout << "effective size " << resp.second.effective_size << std::endl;

            // TODO send the address resp.second.addr to the directory
            //co_await m.get().send_rest_request(DIR_PUT_OBJ_REQ, req);
        }
        else {
            // TODO send the request to the directory

        }
        co_return;
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
