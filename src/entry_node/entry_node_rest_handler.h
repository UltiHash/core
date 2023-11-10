//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_REST_HANDLER_H
#define CORE_ENTRY_NODE_REST_HANDLER_H

#include <iostream>
#include <memory>
#include "common/protocol_handler.h"
#include <common/log.h>
#include "network/client.h"

// HTTP
#include "entry_node/rest/http/http_request.h"
#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/models/put_object_response.h"
#include "entry_node/rest/http/models/get_object_response.h"
#include "entry_node/rest/http/models/create_bucket_response.h"
#include "entry_node/rest/http/models/init_multi_part_upload_response.h"
#include "entry_node/rest/http/models/multi_part_upload_response.h"
#include "entry_node/rest/http/models/complete_multi_part_upload_response.h"
#include "entry_node/rest/http/models/abort_multi_part_upload_response.h"
#include "entry_node/rest/http/models/list_buckets_response.h"
#include "entry_node/rest/http/models/get_object_attributes_response.h"
#include "entry_node/rest/http/models/list_objectsv2_response.h"
#include "entry_node/rest/http/models/list_objects_response.h"
#include "entry_node/rest/http/models/delete_bucket_response.h"
#include "entry_node/rest/http/models/delete_object_response.h"
#include "entry_node/rest/http/models/delete_objects_response.h"
#include "entry_node/rest/http/models/list_multi_part_uploads_response.h"
#include "entry_node/rest/http/models/get_bucket_response.h"
#include "entry_node/rest/http/models/delete_objects_request.h"

// UTILS
#include "entry_node/rest/utils/parser/xml_parser.h"
#include "entry_node/rest/utils/string/string_utils.h"
#include "entry_node/rest/utils/hashing/hash.h"

namespace uh::cluster {

    namespace http = rest::http;


class entry_node_rest_handler {
public:

    entry_node_rest_handler (std::vector <client>& dedupe_nodes, std::vector <client>& directory_nodes):
        m_dedupe_nodes (dedupe_nodes),
        m_directory_nodes (directory_nodes)
    {}

    coro < std::unique_ptr<http::http_response> > handle (rest::http::http_request& req)
    {
        auto body_size = req.get_body_size();
        const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

        std::unique_ptr<http::http_response> res;
//        const auto start = std::chrono::steady_clock::now();

        switch (req.get_request_name())
        {
            case rest::http::http_request_type::CREATE_BUCKET:
                res = co_await handle_create_bucket(req);
                break;
            case http::http_request_type::GET_BUCKET:
                res = co_await handle_get_bucket(req);
                break;
            case http::http_request_type::LIST_BUCKETS:
                res = co_await handle_list_buckets(req);
                break;
            case http::http_request_type::DELETE_BUCKET:
                res = co_await handle_delete_bucket(req);
                break;
            case http::http_request_type::DELETE_OBJECTS:
                res = handle_delete_objects(req);
                break;
            case http::http_request_type::PUT_OBJECT:
                res = co_await handle_put_object(req);
                break;
            case http::http_request_type::GET_OBJECT:
                res = co_await handle_get_object(req);
                break;
            case http::http_request_type::DELETE_OBJECT:
                res = handle_delete_object(req);
                break;
            case http::http_request_type::LIST_OBJECTS_V2:
                res = co_await handle_list_objects_v2(req);
                break;
            case http::http_request_type::LIST_OBJECTS:
                res = co_await handle_list_objects(req);
                break;
            case http::http_request_type::GET_OBJECT_ATTRIBUTES:
                res = handle_get_object_attributes(req);
                break;
            case http::http_request_type::INIT_MULTIPART_UPLOAD:
                res = handle_init_mp_upload(req);
                break;
            case http::http_request_type::MULTIPART_UPLOAD:
                res = handle_mp_upload(req);
                break;
            case http::http_request_type::COMPLETE_MULTIPART_UPLOAD:
                res = co_await handle_complete_mp_upload(req);
                break;
            case http::http_request_type::ABORT_MULTIPART_UPLOAD:
                res = handle_abort_mp_upload(req);
                break;
            case http::http_request_type::LIST_MULTI_PART_UPLOADS:
                res = handle_list_mp_uploads(req);
                break;
            default:
                throw std::runtime_error("request not supported by the backend yet.");
        }

//        const auto stop = std::chrono::steady_clock::now ();
//        const std::chrono::duration <double> duration = stop - start;
//        LOG_INFO() << "duration " << duration.count() << " s";
//        const auto bandwidth = size_mb / duration.count();
//        LOG_INFO() << "bandwidth " << bandwidth << " MB/s";

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_create_bucket (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::create_object_response> res = std::make_unique<http::model::create_object_response>(req);

        try
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
        }
        catch(const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error();
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_delete_bucket (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::delete_bucket_response> res;

        try
        {
            res = std::make_unique<http::model::delete_bucket_response>(req);

            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();
            co_await m_dir.get().send_directory_message (DIR_DELETE_BUCKET_REQ, dir_req);

            const auto h_dir = co_await m_dir.get().recv_header();
            if(h_dir.type == FAILURE) [[unlikely]]
            {
                std::string msg;
                msg.resize(h_dir.size);
                m_dir.get().register_read_buffer(msg);
                co_await m_dir.get().recv_buffers(h_dir);
                throw std::runtime_error("Failed to delete bucket.\nError: \n" + msg);
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_found, 11});
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_get_bucket (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::get_bucket_response> res;
        try
        {
            res = std::make_unique<http::model::get_bucket_response>(req);

            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
            co_await m_dir.get().send(DIR_LIST_BUCKET_REQ, {});
            const auto h_dir = co_await m_dir.get().recv_header();

            if(h_dir.type == DIR_LIST_BUCKET_RESP) [[likely]]
            {
                auto list_buckets_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);
                for (const auto& bucket: list_buckets_res.entities)
                {
                    if (bucket == req.get_URI().get_bucket_id())
                        res->add_bucket(bucket);
                }
                if (res->get_bucket().empty())
                    throw std::runtime_error("Bucket: " + req.get_URI().get_bucket_id() + " doesn't exist.");
            }
            if(h_dir.type == FAILURE) [[unlikely]]
            {
                std::string msg;
                msg.resize(h_dir.size);
                m_dir.get().register_read_buffer(msg);
                co_await m_dir.get().recv_buffers(h_dir);
                throw std::runtime_error("Failed to find bucket.\nError: \n" + msg);
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_found, 11});
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_list_buckets (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::list_buckets_response> res;
        try
        {
            res = std::make_unique<http::model::list_buckets_response>(req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
            co_await m_dir.get().send(DIR_LIST_BUCKET_REQ, {});
            const auto h_dir = co_await m_dir.get().recv_header();

            if(h_dir.type == DIR_LIST_BUCKET_RESP) [[likely]]
            {
                auto list_buckets_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);
                for (const auto& bucket: list_buckets_res.entities)
                {
                    res->add_bucket(bucket);
                }
            }
            if(h_dir.type == FAILURE) [[unlikely]]
            {
                std::string msg;
                msg.resize(h_dir.size);
                m_dir.get().register_read_buffer(msg);
                co_await m_dir.get().recv_buffers(h_dir);
                throw std::runtime_error("Failed to list buckets.\nError: \n" + msg);
            }

        }
        catch (const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_found, 11});
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_put_object (rest::http::http_request& req)
    {
        std::unique_ptr<http::model::put_object_response> res;

        try
        {
            res = std::make_unique<http::model::put_object_response>(req);
            auto body_size = req.get_body_size();
            const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

            auto m_dedup = m_dedupe_nodes.at(get_round_robin_index(m_dedupe_node_index, m_dedupe_nodes.size())).acquire_messenger();
            std::pair <messenger::header, dedupe_response> resp;
            if(body_size > 0) [[likely]] {
                co_await m_dedup.get().send (DEDUPE_REQ, req.get_body());
                const auto h_dedup = co_await m_dedup.get().recv_header();
                resp = co_await m_dedup.get().recv_dedupe_response(h_dedup);
            } else [[unlikely]] {
                resp = std::make_pair<messenger::header, dedupe_response>({.type = DEDUPE_RESP, .size = 0},
                                                                          {.effective_size = 0, .addr = address()});
            }

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
            }

            rest::utils::hashing::MD5 md5;
            res->set_etag(md5.calculateMD5(req.get_body()));
        }
        catch(const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_found, 11});
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_get_object (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::get_object_response> res;

        try
        {

            std::chrono::time_point <std::chrono::steady_clock> timer;
            const auto start = std::chrono::steady_clock::now();

            res = std::make_unique<http::model::get_object_response>(req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();
            dir_req.object_key = std::make_unique <std::string> (req.get_URI().get_object_key());

            co_await m_dir.get().send_directory_message (DIR_GET_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();

            ospan <char> buffer (h_dir.size);
            std::string msg;

            switch (h_dir.type)
            {
                case DIR_GET_OBJ_RESP:
                    m_dir.get().register_read_buffer(buffer);
                    co_await m_dir.get().recv_buffers(h_dir);
                    res->set_body(std::string(buffer.data.get(), buffer.size));
                    break;

                case FAILURE:
                    msg.resize(h_dir.size);
                    m_dir.get().register_read_buffer(msg);
                    co_await m_dir.get().recv_buffers(h_dir);
                    throw std::runtime_error("Failed to retrieve object " + dir_req.bucket_id + "/" + *dir_req.object_key + " from the directory.\n" + "Error: \n" + msg);

                default:
                    throw std::runtime_error("unexpected internal server error");
            }

            const auto stop = std::chrono::steady_clock::now ();
            const std::chrono::duration <double> duration = stop - start;
            const auto size = h_dir.size / static_cast <double> (1024ul * 1024ul);
            const auto bandwidth = size / duration.count();
            LOG_INFO() << "retrieval duration " << duration.count() << " s";
            LOG_INFO() << "retrieval bandwidth " << bandwidth << " MB/s";

        }
        catch (const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_found, 11});
        }

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_get_object_attributes (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::get_object_attributes_response> res;

        res = std::make_unique<http::model::get_object_attributes_response>(req);
        res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_implemented, 11});

        return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_list_objects_v2 (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::list_objectsv2_response> res;

        try
        {
            res = std::make_unique<http::model::list_objectsv2_response>(req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();

            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();

            co_await m_dir.get().send_directory_message (DIR_LIST_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();

            ospan <char> buffer (h_dir.size);
            std::string msg;
            directory_lst_entities_message list_objects_res;

            switch (h_dir.type)
            {
                case DIR_LIST_OBJ_RESP:
                    list_objects_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);

                    for (const auto& content : list_objects_res.entities)
                    {
                        res->add_content(content);
                    }
                    break;

                case FAILURE:
                    msg.resize(h_dir.size);
                    m_dir.get().register_read_buffer(msg);
                    co_await m_dir.get().recv_buffers(h_dir);
                    throw std::runtime_error("Failed to list objects of bucket: " + dir_req.bucket_id + "\n" + "Error: \n" + msg);

                default:
                    throw std::runtime_error("unexpected internal server error");
            }

        }
        catch(const std::exception &e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_found, 11});
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_list_objects (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::list_objects_response> res;

        try
        {
            res = std::make_unique<http::model::list_objects_response>(req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())).acquire_messenger();

            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();

            co_await m_dir.get().send_directory_message (DIR_LIST_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();

            ospan <char> buffer (h_dir.size);
            std::string msg;
            directory_lst_entities_message list_objects_res;

            switch (h_dir.type)
            {
                case DIR_LIST_OBJ_RESP:
                    list_objects_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);

                    for (const auto& content : list_objects_res.entities)
                    {
                        res->add_content(content);
                    }
                    res->add_name(req.get_URI().get_bucket_id());
                    break;

                case FAILURE:
                    msg.resize(h_dir.size);
                    m_dir.get().register_read_buffer(msg);
                    co_await m_dir.get().recv_buffers(h_dir);
                    throw std::runtime_error("Failed to list objects of bucket: " + dir_req.bucket_id + "\n" + "Error: \n" + msg);

                default:
                    throw std::runtime_error("unexpected internal server error");
            }

        }
        catch(const std::exception &e)
        {
            LOG_ERROR() << e.what();
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::internal_server_error, 11});
        }

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_init_mp_upload (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::init_multi_part_upload_response> res;
        try
        {
            res = std::make_unique<http::model::init_multi_part_upload_response>(req);
            res->set_upload_id(req.get_eTag());
        }
        catch(const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error();
        }

        return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_mp_upload (rest::http::http_request& req)
    {

        std::unique_ptr<http::model::multi_part_upload_response> res;
        try
        {
            res = std::make_unique<http::model::multi_part_upload_response>(req);

            rest::utils::hashing::MD5 md5;
            res->set_etag(md5.calculateMD5(req.get_body()));
        }
        catch(const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error();
        }

        return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_complete_mp_upload (rest::http::http_request& req)
    {
        std::unique_ptr<http::model::complete_multi_part_upload_response> res;
        try
        {

            res = std::make_unique<http::model::complete_multi_part_upload_response>(req);
            auto body_size = req.get_body_size();

            std::chrono::time_point <std::chrono::steady_clock> timer;
            const auto start = std::chrono::steady_clock::now();
            const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

            auto m_dedup = m_dedupe_nodes.at(get_round_robin_index(m_dedupe_node_index, m_dedupe_nodes.size())).acquire_messenger();
            std::pair <messenger::header, dedupe_response> resp;
            if(body_size > 0) [[likely]] {
                co_await m_dedup.get().send (DEDUPE_REQ, req.get_body());
                const auto h_dedup = co_await m_dedup.get().recv_header();
                resp = co_await m_dedup.get().recv_dedupe_response(h_dedup);
            } else [[unlikely]] {
                resp = std::make_pair<messenger::header, dedupe_response>({.type = DEDUPE_RESP, .size = 0},
                                                                          {.effective_size = 0, .addr = address()});
            }

            auto effective_size = static_cast <double> (resp.second.effective_size) / static_cast <double> (1024ul * 1024ul);
            auto space_saving = 1.0 - static_cast <double> (resp.second.effective_size) / static_cast <double> (body_size);

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
            }

            rest::utils::hashing::MD5 md5;
            res->set_etag(md5.calculateMD5(req.get_body()));

            LOG_INFO() << "original size " << size_mb << " MB";
            LOG_INFO() << "effective size " << effective_size << " MB";
            LOG_INFO() << "space saving " << space_saving;
            const auto stop = std::chrono::steady_clock::now ();
            const std::chrono::duration <double> duration = stop - start;
            const auto bandwidth = size_mb / duration.count();
            LOG_INFO() << "integration duration " << duration.count() << " s";
            LOG_INFO() << "integration bandwidth " << bandwidth << " MB/s";

        }
        catch(const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error();
        }

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_abort_mp_upload (const rest::http::http_request& req)
    {
        std::unique_ptr<http::model::abort_multi_part_upload_response> res;
        try
        {
            res = std::make_unique<http::model::abort_multi_part_upload_response>(req);
        }
        catch(const std::exception& e)
        {
            LOG_ERROR() << e.what();
            res->set_error();
        }

        return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_delete_object (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::delete_object_response> res = std::make_unique<http::model::delete_object_response>(req);
        res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_implemented, 11});

        return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_list_mp_uploads (const rest::http::http_request& req)
    {

        std::unique_ptr<http::model::list_multi_part_uploads_response> res = std::make_unique<http::model::list_multi_part_uploads_response>(req);
        res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_implemented, 11});

        return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_delete_objects (rest::http::http_request& req)
    {

        std::unique_ptr<http::model::delete_objects_response> res;

        try
        {
            res = std::make_unique<http::model::delete_objects_response>(req);
//            rest::utils::parser::xml_parser parsed_xml(req.get_body());
//
//            std::vector<rest::http::model::object> objects_container;
//
//            auto object_nodes_set = parsed_xml.get_nodes_from_path("/Delete/Object");
//            for (const auto& objectNode : object_nodes_set)
//            {
//                objects_container.emplace_back(objectNode.node().child("Key").child_value(), objectNode.node().child("VersionId").child_value());
//            }

        }
        catch (const std::exception& e)
        {
            res->set_error(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::not_implemented, 11});
        }

        return std::move(res);
    }

    [[nodiscard]] client& get_recovery_director () const {
        return m_directory_nodes.at(0);
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
