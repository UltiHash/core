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

    coro <http::response<http::string_body>> handle (const parsed_request_wrapper& req) {

        const auto size_mb = static_cast <double> (req.body.size()) / static_cast <double> (1024ul * 1024ul);

        http::response<http::string_body> res {http::status::ok, 11};
        res.set(http::field::server, "UltiHash v0.2.0");
        res.set(http::field::content_type, "text/html");

        std::chrono::time_point <std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now ();

        switch (req.req_type) {
            case put_object:
            case close_multi_part:
                co_await handle_put_object (req, res);
                break;
            case get_object:
                co_await handle_get_object (req, res);
                break;
            case create_bucket:
                co_await handle_create_bucket (req, res);
                break;
            default:
                throw std::invalid_argument ("Not supported request type");
        }

        const auto stop = std::chrono::steady_clock::now ();
        const std::chrono::duration <double> duration = stop - start;
        std::cout << "duration " << duration.count() << " s" << std::endl;
        const auto bandwidth = size_mb / duration.count();
        std::cout << "bandwidth " << bandwidth << " MB/s" << std::endl;

        res.prepare_payload();
        co_return res;
    }

    coro <void> handle_put_object (const parsed_request_wrapper& req, http::response<http::string_body>& res) {
        const auto size_mb = static_cast <double> (req.body.size()) / static_cast <double> (1024ul * 1024ul);

        auto m_dedup = m_dedupe_nodes.at(get_round_robin_index(m_dedupe_node_index, m_dedupe_nodes.size())).acquire_messenger();
        co_await m_dedup.get().send (DEDUPE_REQ, req.body);
        const auto h_dedup = co_await m_dedup.get().recv_header();
        auto resp = co_await m_dedup.get().recv_dedupe_response(h_dedup);

        auto effective_size = static_cast <double> (resp.second.effective_size) / static_cast <double> (1024ul * 1024ul);
        auto space_saving = 1.0 - static_cast <double> (resp.second.effective_size) / static_cast <double> (req.body.size());

        std::cout << "original size " << size_mb << " MB" << std::endl;
        std::cout << "effective size " << effective_size << " MB" << std::endl;
        std::cout << "space saving " << space_saving << std::endl;

        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
        const directory_message dir_req {
                .bucket_id = req.bucket_id,
                .object_key = std::make_unique <std::string> (req.object_key),
                .addr = std::make_unique <address> (std::move (resp.second.addr)),
        };
        co_await m_dir.get().send_directory_message (DIR_PUT_OBJ_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();
        if(h_dir.type == FAILURE) [[unlikely]] {
            std::string msg;
            msg.resize(h_dir.size);
            m_dir.get().register_read_buffer(msg);
            co_await m_dir.get().recv_buffers(h_dir);
            throw std::runtime_error("Failed to add the fragment address of object " + dir_req.bucket_id + "/" + *dir_req.object_key + " to the directory.\n" + "Error: \n" + msg);
            //TODO: consider using custom exceptions to indicate if and how the error gets communicated to the HTTP client.
        }

        if (req.req_type == close_multi_part)
        {
            res.set(boost::beast::http::field::transfer_encoding, "chunked");
            res.set(boost::beast::http::field::connection, "close");
            res.set(boost::beast::http::field::content_type, "application/xml");
            res.body() =  std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                      "<CompleteMultipartUploadResult>\n"
                                      "<Location>string</Location>\n"
                                      "<Bucket>" + req.bucket_id +"</Bucket>\n"
                                                                  "<Key>" + req.object_key + "</Key>\n"
                                                                                             "<ETag>string</ETag>\n"
                                                                                             "</CompleteMultipartUploadResult>");
        }
    }

    coro <void> handle_get_object (const parsed_request_wrapper& req, http::response<http::string_body>& res) {
        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
        directory_message dir_req;
        dir_req.bucket_id = req.bucket_id;
        dir_req.object_key = std::make_unique <std::string> (req.object_key);
        co_await m_dir.get().send_directory_message (DIR_GET_OBJ_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();
        if(h_dir.type == DIR_GET_OBJ_RESP) [[likely]] {
            ospan <char> buffer (h_dir.size);
            m_dir.get().register_read_buffer(buffer);
            co_await m_dir.get().recv_buffers(h_dir);
            res.body() = std::string(buffer.data.get(), buffer.size);
        } else if (h_dir.type == FAILURE){
            std::string msg;
            msg.resize(h_dir.size);
            m_dir.get().register_read_buffer(msg);
            co_await m_dir.get().recv_buffers(h_dir);
            throw std::runtime_error("Failed to retreive object " + dir_req.bucket_id + "/" + *dir_req.object_key + " from the directory.\n" + "Error: \n" + msg);
            //TODO: consider using custom exceptions to indicate if and how the error gets communicated to the HTTP client.
        }
    }

    coro <void> handle_create_bucket (const parsed_request_wrapper& req, http::response<http::string_body>& res) {
        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
        directory_message dir_req;
        dir_req.bucket_id = req.bucket_id;
        co_await m_dir.get().send_directory_message (DIR_PUT_BUCKET_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();
        if(h_dir.type == FAILURE) [[unlikely]] {
            std::string msg;
            msg.resize(h_dir.size);
            m_dir.get().register_read_buffer(msg);
            co_await m_dir.get().recv_buffers(h_dir);
            throw std::runtime_error("Failed to add the bucket " + dir_req.bucket_id + " to the directory.\n" + "Error: \n" + msg);
        }
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
