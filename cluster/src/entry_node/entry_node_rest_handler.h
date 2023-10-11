//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_REST_HANDLER_H
#define CORE_ENTRY_NODE_REST_HANDLER_H

#include "common/protocol_handler.h"
#include "network/client.h"
#include <iostream>
#include "entry_node/rest/http/http_request.h"
#include "entry_node/rest/http/http_response.h"

namespace uh::cluster {

    namespace http = rest::http;

class entry_node_rest_handler {
public:

    entry_node_rest_handler (std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes):
        m_dedupe_nodes (dedupe_nodes),
        m_directory_nodes (directory_nodes)
    {}

    coro < http::http_response > handle (const rest::http::http_request& req)
    {

        http::http_response res(req);

        auto body_size = req.get_body_size();
        const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

        std::chrono::time_point <std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now ();

        switch (req.get_request_name())
        {
            case http::http_request_type::CREATE_BUCKET:
                co_await handle_create_bucket(req, res);
                break;
            case http::http_request_type::PUT_OBJECT:
            case http::http_request_type::COMPLETE_MULTIPART_UPLOAD:
                co_await handle_put_object(req, res);
                break;
            case http::http_request_type::GET_OBJECT:
                co_await handle_get_object(req, res);
                break;
            case http::http_request_type::INIT_MULTIPART_UPLOAD:
                handle_init_mp_upload(req, res);
                break;
            case http::http_request_type::MULTIPART_UPLOAD:
                handle_multipart_upload(res);
                break;
            case http::http_request_type::ABORT_MULTIPART_UPLOAD:
                handle_abort_mp_upload(res);
                break;
            default:
                throw std::invalid_argument("Not supported request type");
        }

        const auto stop = std::chrono::steady_clock::now ();
        const std::chrono::duration <double> duration = stop - start;
        std::cout << "duration " << duration.count() << " s" << std::endl;
        const auto bandwidth = size_mb / duration.count();
        std::cout << "bandwidth " << bandwidth << " MB/s" << std::endl;

        res.get_underlying_object().prepare_payload();

        co_return std::move(res);
    }

    void handle_init_mp_upload (const rest::http::http_request& req, rest::http::http_response& res)
    {
        auto& underlying_res = res.get_underlying_object();

        underlying_res.set(boost::beast::http::field::connection, "keep-alive");
        underlying_res.set(boost::beast::http::field::content_type, "application/xml");

        // TODO: For now, we use fixed upload id for a client
        underlying_res.body() =  std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                  "<InitiateMultipartUploadResult>\n"
                                  "<Bucket>"+ req.get_URI().get_bucket_id() + "</Bucket>\n"
                                  "<Key>" + req.get_URI().get_object_key() + "</Key>\n"
                                  "<UploadId>first-upload</UploadId>\n"
                                  "</InitiateMultipartUploadResult>");
   }

    void handle_multipart_upload (rest::http::http_response& res)
    {
        auto& underlying_res = res.get_underlying_object();

        underlying_res.set(boost::beast::http::field::connection, "keep-alive");
        underlying_res.set(boost::beast::http::field::content_type, "application/xml");
        underlying_res.set(boost::beast::http::field::etag, "ThisistheCustomEtag");
    }

    void handle_abort_mp_upload (rest::http::http_response& res)
    {
        res.set_response_object(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::no_content, 11});
    }

    coro <void> handle_put_object (const rest::http::http_request& req, rest::http::http_response& res)
    {
        auto body_size = req.get_body_size();
        const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

        auto m_dedup = m_dedupe_nodes.at(get_round_robin_index(m_dedupe_node_index, m_dedupe_nodes.size())).acquire_messenger();
        co_await m_dedup.get().send (DEDUPE_REQ, req.get_body());
        const auto h_dedup = co_await m_dedup.get().recv_header();
        auto resp = co_await m_dedup.get().recv_dedupe_response(h_dedup);

        auto effective_size = static_cast <double> (resp.second.effective_size) / static_cast <double> (1024ul * 1024ul);
        auto space_saving = 1.0 - static_cast <double> (resp.second.effective_size) / static_cast <double> (body_size);

        std::cout << "original size " << size_mb << " MB" << std::endl;
        std::cout << "effective size " << effective_size << " MB" << std::endl;
        std::cout << "space saving " << space_saving << std::endl;

        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
        const directory_message dir_req
        {
                .bucket_id = req.get_URI().get_bucket_id(),
                .object_key = std::make_unique <std::string> (req.get_URI().get_object_key()),
                .addr = std::make_unique <address> (std::move (resp.second.addr)),
        };

        co_await m_dir.get().send_directory_message (DIR_PUT_OBJ_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();
        if(h_dir.type == FAILURE) [[unlikely]]
        {
            std::string msg;
            msg.resize(h_dir.size);
            m_dir.get().register_read_buffer(msg);
            co_await m_dir.get().recv_buffers(h_dir);
            throw std::runtime_error("Failed to add the fragment address of object " + dir_req.bucket_id + "/" + *dir_req.object_key + " to the directory.\n" + "Error: \n" + msg);
            //TODO: consider using custom exceptions to indicate if and how the error gets communicated to the HTTP client.
        }

        if (req.get_request_name() == rest::http::http_request_type::COMPLETE_MULTIPART_UPLOAD)
        {
            auto& underlying_res = res.get_underlying_object();

            underlying_res.set(boost::beast::http::field::transfer_encoding, "chunked");
            underlying_res.set(boost::beast::http::field::connection, "close");
            underlying_res.set(boost::beast::http::field::content_type, "application/xml");
            underlying_res.body() =  std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                      "<CompleteMultipartUploadResult>\n"
                                      "<Location>string</Location>\n"
                                      "<Bucket>" + req.get_URI().get_bucket_id() +"</Bucket>\n"
                                      "<Key>" + req.get_URI().get_object_key() + "</Key>\n"
                                      "<ETag>string</ETag>\n"
                                      "</CompleteMultipartUploadResult>");
        }

        co_return;
    }

    coro <void> handle_create_bucket (const rest::http::http_request& req, rest::http::http_response& res)
    {
        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
        directory_message dir_req;
        dir_req.bucket_id = req.get_URI().get_bucket_id();
        co_await m_dir.get().send_directory_message (DIR_PUT_BUCKET_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();
        if(h_dir.type == FAILURE) [[unlikely]]
        {
            std::string msg;
            msg.resize(h_dir.size);
            m_dir.get().register_read_buffer(msg);
            co_await m_dir.get().recv_buffers(h_dir);
            throw std::runtime_error("Failed to add the bucket " + dir_req.bucket_id + " to the directory.\n" + "Error: \n" + msg);
        }

        co_return;
    }

    coro <void> handle_get_object (const rest::http::http_request& req, rest::http::http_response& res)
    {
        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
        directory_message dir_req;
        dir_req.bucket_id = req.get_URI().get_bucket_id();
        dir_req.object_key = std::make_unique <std::string> (req.get_URI().get_object_key());

        co_await m_dir.get().send_directory_message (DIR_GET_OBJ_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();

        if(h_dir.type == DIR_GET_OBJ_RESP) [[likely]]
        {
            ospan <char> buffer (h_dir.size);
            m_dir.get().register_read_buffer(buffer);
            co_await m_dir.get().recv_buffers(h_dir);
            res.get_underlying_object().body() = std::string(buffer.data.get(), buffer.size);
        } else if (h_dir.type == FAILURE)
        {
            std::string msg;
            msg.resize(h_dir.size);
            m_dir.get().register_read_buffer(msg);
            co_await m_dir.get().recv_buffers(h_dir);
            throw std::runtime_error("Failed to retreive object " + dir_req.bucket_id + "/" + *dir_req.object_key + " from the directory.\n" + "Error: \n" + msg);
            //TODO: consider using custom exceptions to indicate if and how the error gets communicated to the HTTP client.
        }

        co_return;
    }

private:

    static size_t get_round_robin_index (std::atomic <size_t>& current_index, const size_t total_size)
    {
        auto index = current_index.load();
        auto new_val = (index + 1) % total_size;

        while (!current_index.compare_exchange_weak (index, new_val))
        {
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
