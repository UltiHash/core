//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_REST_HANDLER_H
#define CORE_ENTRY_NODE_REST_HANDLER_H

#include <iostream>
#include <memory>
#include "common/metrics_handler.h"
#include "common/log.h"
#include "network/client.h"
#include "network/network_traits.h"

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
#include "entry_node/rest/http/models/custom_error_response_exception.h"

// UTILS
#include "entry_node/rest/utils/parser/xml_parser.h"
#include "entry_node/rest/utils/string/string_utils.h"
#include "entry_node/rest/utils/hashing/hash.h"
#include "entry_node/rest/utils/containers/internal_server_state.h"

namespace uh::cluster {

namespace http = rest::http;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace b_http = beast::http;           // from <boost/beast/http.hpp>

class entry_node_rest_handler : protected metrics_handler {
public:

    entry_node_rest_handler (std::shared_ptr <boost::asio::io_context> ioc, std::vector <std::shared_ptr <client>>& dedupe_nodes, std::vector <std::shared_ptr <client>>& directory_nodes, entry_node_config config):
            metrics_handler(config.rest_server_conf),
            m_ioc (std::move (ioc)),
            m_dedupe_nodes (dedupe_nodes),
            m_directory_nodes (directory_nodes),
            m_entry_node_config(config),

            m_req_counters(add_counter_family("uh_en_requests", "number of HTTP requests handled by the entry node")),

            m_reqs_create_bucket(m_req_counters.Add({{"request_type", "CREATE_BUCKET"}})),
            m_reqs_get_bucket(m_req_counters.Add({{"request_type", "GET_BUCKET"}})),
            m_reqs_list_buckets(m_req_counters.Add({{"request_type", "LIST_BUCKETS"}})),
            m_reqs_delete_bucket(m_req_counters.Add({{"request_type", "DELETE_BUCKET"}})),
            m_reqs_delete_objects(m_req_counters.Add({{"request_type", "DELETE_OBJECTS"}})),
            m_reqs_put_object(m_req_counters.Add({{"request_type", "PUT_OBJECT"}})),
            m_reqs_get_object(m_req_counters.Add({{"request_type", "GET_OBJECT"}})),
            m_reqs_delete_object(m_req_counters.Add({{"request_type", "DELETE_OBJECT"}})),
            m_reqs_list_objects_v2(m_req_counters.Add({{"request_type", "LIST_OBJECTS_V2"}})),
            m_reqs_list_objects(m_req_counters.Add({{"request_type", "LIST_OBJECTS"}})),
            m_reqs_get_object_attributes(m_req_counters.Add({{"request_type", "GET_OBJECT_ATTRIBUTES"}})),
            m_reqs_init_multipart_upload(m_req_counters.Add({{"request_type", "INIT_MULTIPART_UPLOAD"}})),
            m_reqs_multiplart_upload(m_req_counters.Add({{"request_type", "MULTIPART_UPLOAD"}})),
            m_reqs_complete_multipart_upload(m_req_counters.Add({{"request_type", "COMPLETE_MULTIPART_UPLOAD"}})),
            m_reqs_abort_multipart_upload(m_req_counters.Add({{"request_type", "ABORT_MULTIPART_UPLOAD"}})),
            m_reqs_list_multi_part_uploads(m_req_counters.Add({{"request_type", "LIST_MULTI_PART_UPLOADS"}})),
            m_reqs_invalid(m_req_counters.Add({{"request_type", "INVALID"}}))
    {
        init();
    }

    coro < std::unique_ptr<http::http_response> > handle (http::http_request& req, rest::utils::state& state)
    {
        auto body_size = req.get_body_size();
        const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

        std::unique_ptr<http::http_response> res;
//        const auto start = std::chrono::steady_clock::now();

        switch (req.get_request_name())
        {
            case http::http_request_type::CREATE_BUCKET:
                m_reqs_create_bucket.Increment();
                res = co_await handle_create_bucket(req);
                break;
            case http::http_request_type::GET_BUCKET:
                m_reqs_get_bucket.Increment();
                res = co_await handle_get_bucket(req);
                break;
            case http::http_request_type::LIST_BUCKETS:
                m_reqs_list_buckets.Increment();
                res = co_await handle_list_buckets(req);
                break;
            case http::http_request_type::DELETE_BUCKET:
                m_reqs_delete_bucket.Increment();
                res = co_await handle_delete_bucket(req);
                break;
            case http::http_request_type::DELETE_OBJECTS:
                m_reqs_delete_objects.Increment();
                res = co_await handle_delete_objects(req);
                break;
            case http::http_request_type::PUT_OBJECT:
                m_reqs_put_object.Increment();
                res = co_await handle_put_object(req);
                break;
            case http::http_request_type::GET_OBJECT:
                m_reqs_get_object.Increment();
                res = co_await handle_get_object(req);
                break;
            case http::http_request_type::DELETE_OBJECT:
                m_reqs_delete_object.Increment();
                res = co_await handle_delete_object(req);
                break;
            case http::http_request_type::LIST_OBJECTS_V2:
                m_reqs_list_objects_v2.Increment();
                res = co_await handle_list_objects_v2(req);
                break;
            case http::http_request_type::LIST_OBJECTS:
                m_reqs_list_objects.Increment();
                res = co_await handle_list_objects(req);
                break;
            case http::http_request_type::GET_OBJECT_ATTRIBUTES:
                m_reqs_get_object_attributes.Increment();
                res = handle_get_object_attributes(req);
                break;
            case http::http_request_type::INIT_MULTIPART_UPLOAD:
                m_reqs_init_multipart_upload.Increment();
                res = handle_init_mp_upload(req);
                break;
            case http::http_request_type::MULTIPART_UPLOAD:
                m_reqs_multiplart_upload.Increment();
                res = handle_mp_upload(req);
                break;
            case http::http_request_type::COMPLETE_MULTIPART_UPLOAD:
                m_reqs_complete_multipart_upload.Increment();
                res = co_await handle_complete_mp_upload(req, state);
                break;
            case http::http_request_type::ABORT_MULTIPART_UPLOAD:
                m_reqs_abort_multipart_upload.Increment();
                res = handle_abort_mp_upload(req);
                break;
            case http::http_request_type::LIST_MULTI_PART_UPLOADS:
                m_reqs_list_multi_part_uploads.Increment();
                res = handle_list_mp_uploads(req, state);
                break;
            default:
                m_reqs_invalid.Increment();
                throw std::runtime_error("request not supported by the backend yet.");
        }

//        const auto stop = std::chrono::steady_clock::now ();
//        const std::chrono::duration <double> duration = stop - start;
//        LOG_INFO() << "duration " << duration.count() << " s";
//        const auto bandwidth = size_mb / duration.count();
//        LOG_INFO() << "bandwidth " << bandwidth << " MB/s";

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_create_bucket (const http::http_request& req)
    {

        std::unique_ptr<http::model::create_object_response> res = std::make_unique<http::model::create_object_response>(req);
        auto bucket_id = req.get_URI().get_bucket_id();

        try
        {
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = bucket_id;
            co_await m_dir.get().send_directory_message (DIR_PUT_BUCKET_REQ, dir_req);

            co_await m_dir.get().recv_header();
        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << "Failed to add the bucket " << bucket_id << " to the directory: " << e;
            throw http::model::custom_error_response_exception(b_http::status::not_found);
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_delete_bucket (const http::http_request& req)
    {

        std::unique_ptr<http::model::delete_bucket_response> res;

        try
        {
            res = std::make_unique<http::model::delete_bucket_response>(req);

            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();
            co_await m_dir.get().send_directory_message (DIR_DELETE_BUCKET_REQ, dir_req);

            co_await m_dir.get().recv_header();
        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << "Failed to delete bucket: " << e;
            switch (*e.error())
            {
                case error::bucket_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
                case error::bucket_not_empty:
                    throw http::model::custom_error_response_exception(b_http::status::conflict, http::model::error::bucket_not_empty);
                default:
                    throw http::model::custom_error_response_exception(b_http::status::internal_server_error);
            }
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_get_bucket (const http::http_request& req)
    {

        std::unique_ptr<http::model::get_bucket_response> res;
        auto req_bucket_id = req.get_URI().get_bucket_id();

        try
        {
            res = std::make_unique<http::model::get_bucket_response>(req);

            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
            co_await m_dir.get().send(DIR_LIST_BUCKET_REQ, {});

            auto hdr = co_await m_dir.get().recv_header();

            if(hdr.type == DIR_LIST_BUCKET_RESP) [[likely]]
            {
                auto list_buckets_res = co_await m_dir.get().recv_directory_list_entities_message(hdr);
                for (const auto& bucket: list_buckets_res.entities)
                {
                    if (bucket == req_bucket_id)
                    {
                        res->add_bucket(bucket);
                        break;
                    }
                }

                if (res->get_bucket().empty())
                {
                    throw error_exception(error::bucket_not_found);
                }
            }
            // TODO throw here
        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << "Failed to get bucket `" << req_bucket_id << "`: " << e;
            switch (*e.error())
            {
                case error::bucket_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
                default:
                    throw http::model::custom_error_response_exception(b_http::status::internal_server_error);
            }
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_list_buckets (const http::http_request& req)
    {

        std::unique_ptr<http::model::list_buckets_response> res;

        res = std::make_unique<http::model::list_buckets_response>(req);
        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
        co_await m_dir.get().send(DIR_LIST_BUCKET_REQ, {});
        const auto h_dir = co_await m_dir.get().recv_header();

        auto list_buckets_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);
        for (const auto& bucket: list_buckets_res.entities)
        {
            res->add_bucket(bucket);
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_put_object (http::http_request& req)
    {
        std::unique_ptr<http::model::put_object_response> res;
        auto req_bucket_id = req.get_URI().get_bucket_id();

        try
        {
            res = std::make_unique<http::model::put_object_response>(req);

            std::chrono::time_point <std::chrono::steady_clock> timer;
            const auto start = std::chrono::steady_clock::now();

            auto body_size = req.get_body_size();
            const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);

            dedupe_response resp {.effective_size = 0};
            if(body_size > 0) [[likely]] {
                std::list<std::string_view> data{req.get_body()};
                resp = co_await integrate_data(data);
            }

            auto effective_size = static_cast <double> (resp.effective_size) / static_cast <double> (1024ul * 1024ul);
            auto space_saving = 1.0 - static_cast <double> (resp.effective_size) / static_cast <double> (body_size);

            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
            const directory_message dir_req
                    {
                            .bucket_id = req.get_URI().get_bucket_id(),
                            .object_key = std::make_unique <std::string> (req.get_URI().get_object_key()),
                            .addr = std::make_unique <address> (std::move (resp.addr)),
                    };

            co_await m_dir.get().send_directory_message (DIR_PUT_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();

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

            res->set_size(size_mb);
            res->set_effective_size(effective_size);
            res->set_space_savings(space_saving);
            res->set_bandwidth(bandwidth);

        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << "Failed to get bucket `" << req_bucket_id << "`: " << e;
            switch (*e.error())
            {
                case error::bucket_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
                default:
                    throw http::model::custom_error_response_exception(b_http::status::internal_server_error);
            }
        }


        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_get_object (const http::http_request& req)
    {

        std::unique_ptr<http::model::get_object_response> res;

        try
        {

            std::chrono::time_point <std::chrono::steady_clock> timer;
            const auto start = std::chrono::steady_clock::now();

            res = std::make_unique<http::model::get_object_response>(req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();
            dir_req.object_key = std::make_unique <std::string> (req.get_URI().get_object_key());

            co_await m_dir.get().send_directory_message (DIR_GET_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();

            ospan <char> buffer (h_dir.size);
            std::string msg;

            m_dir.get().register_read_buffer(buffer);
            co_await m_dir.get().recv_buffers(h_dir);
            res->set_body(std::string(buffer.data.get(), buffer.size));

            const auto stop = std::chrono::steady_clock::now ();
            const std::chrono::duration <double> duration = stop - start;
            const auto size = h_dir.size / static_cast <double> (1024ul * 1024ul);
            const auto bandwidth = size / duration.count();
            LOG_INFO() << "retrieval duration " << duration.count() << " s";
            LOG_INFO() << "retrieval bandwidth " << bandwidth << " MB/s";

            res->set_bandwidth(bandwidth);

        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << e.what();
            switch (*e.error())
            {
                case error::object_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::object_not_found);
                default:
                    throw http::model::custom_error_response_exception(b_http::status::internal_server_error);
            }
        }

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_get_object_attributes (const http::http_request& req)
    {

        std::unique_ptr<http::model::get_object_attributes_response> res;

        res = std::make_unique<http::model::get_object_attributes_response>(req);
        throw http::model::custom_error_response_exception(b_http::status::not_implemented);

        return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_list_objects_v2 (const http::http_request& req)
    {

        std::unique_ptr<http::model::list_objectsv2_response> res;

        try
        {
            res = std::make_unique<http::model::list_objectsv2_response>(req);
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();

            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();

            co_await m_dir.get().send_directory_message (DIR_LIST_OBJ_REQ, dir_req);
            const auto h_dir = co_await m_dir.get().recv_header();

            ospan <char> buffer (h_dir.size);
            std::string msg;
            directory_lst_entities_message list_objects_res;

            list_objects_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);

            for (const auto& content : list_objects_res.entities)
            {
                res->add_content(content);
            }

        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << e.what();
            switch (*e.error())
            {
                case error::bucket_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
                default:
                    throw http::model::custom_error_response_exception(b_http::status::internal_server_error);
            }
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_list_objects (const http::http_request& req)
    {

        std::unique_ptr<http::model::list_objects_response> res;

        res = std::make_unique<http::model::list_objects_response>(req);
        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();

        directory_message dir_req;
        dir_req.bucket_id = req.get_URI().get_bucket_id();

        co_await m_dir.get().send_directory_message (DIR_LIST_OBJ_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();

        ospan <char> buffer (h_dir.size);
        std::string msg;
        directory_lst_entities_message list_objects_res;

        list_objects_res = co_await m_dir.get().recv_directory_list_entities_message(h_dir);

        for (const auto& content : list_objects_res.entities)
        {
            res->add_content(content);
        }
        res->add_name(req.get_URI().get_bucket_id());

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_init_mp_upload (const http::http_request& req)
    {

        std::unique_ptr<http::model::init_multi_part_upload_response> res;
        res = std::make_unique<http::model::init_multi_part_upload_response>(req);
        res->set_upload_id(req.get_eTag());

        return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_mp_upload (http::http_request& req)
    {

        std::unique_ptr<http::model::multi_part_upload_response> res;

        res = std::make_unique<http::model::multi_part_upload_response>(req);

        rest::utils::hashing::MD5 md5;
        res->set_etag(md5.calculateMD5(req.get_body()));

        return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_complete_mp_upload (http::http_request& req, rest::utils::state& state)
    {
        std::unique_ptr<http::model::complete_multi_part_upload_response> res;

        res = std::make_unique<http::model::complete_multi_part_upload_response>(req);

        // acquire the internal parts container
        auto parts_container_ptr = state.get_multipart_container().get_value(req.get_URI().get_query_parameters().at("uploadId"));
        auto body_size = req.get_body_size();

        std::cout << "UPLOAD SIZE: " << req.get_body_size() << std::endl;

        std::chrono::time_point <std::chrono::steady_clock> timer;
        const auto start = std::chrono::steady_clock::now();
        const auto size_mb = static_cast <double> (body_size) / static_cast <double> (1024ul * 1024ul);


        dedupe_response resp {.effective_size = 0};
        if(body_size > 0) [[likely]] {
            std::list<std::string_view> pieces;
            for (const auto &pair: *parts_container_ptr) {
                pieces.emplace_back(pair.second.second);
            }
            resp = co_await integrate_data(pieces);
        }

        auto effective_size = static_cast <double> (resp.effective_size) / static_cast <double> (1024ul * 1024ul);
        auto space_saving = 1.0 - static_cast <double> (resp.effective_size) / static_cast <double> (body_size);

        auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
        const directory_message dir_req
            {
                .bucket_id = req.get_URI().get_bucket_id(),
                .object_key = std::make_unique <std::string> (req.get_URI().get_object_key()),
                .addr = std::make_unique <address> (std::move (resp.addr)),
            };

        co_await m_dir.get().send_directory_message (DIR_PUT_OBJ_REQ, dir_req);
        const auto h_dir = co_await m_dir.get().recv_header();

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

        res->set_size(size_mb);
        res->set_effective_size(effective_size);
        res->set_space_savings(space_saving);
        res->set_bandwidth(bandwidth);

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_abort_mp_upload (const http::http_request& req)
    {
        std::unique_ptr<http::model::abort_multi_part_upload_response> res;
        res = std::make_unique<http::model::abort_multi_part_upload_response>(req);

        return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_delete_object (const http::http_request& req)
    {

        std::unique_ptr<http::model::delete_object_response> res = std::make_unique<http::model::delete_object_response>(req);
        auto bucket_id = req.get_URI().get_bucket_id();

        try
        {
            auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
            directory_message dir_req;
            dir_req.bucket_id = bucket_id;
            dir_req.object_key = std::make_unique <std::string> (req.get_URI().get_object_key());

            co_await m_dir.get().send_directory_message (DIR_DELETE_OBJ_REQ, dir_req);

            co_await m_dir.get().recv_header();
        }
        catch (const error_exception& e)
        {
            switch (*e.error())
            {
                case error::object_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::object_not_found);
                case error::bucket_not_found:
                    throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
                default:
                    throw http::model::custom_error_response_exception(b_http::status::internal_server_error);
            }
        }

        co_return std::move(res);
    }

    std::unique_ptr<http::http_response> handle_list_mp_uploads (const http::http_request& req, rest::utils::state& state)
    {

        std::unique_ptr<http::model::list_multi_part_uploads_response> res = std::make_unique<http::model::list_multi_part_uploads_response>(req);

        auto bucket_name = req.get_URI().get_bucket_id();
        auto ptr = state.get_bucket_multiparts().get_value(bucket_name);
        if (ptr == nullptr)
        {
            throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
        }
        else
        {
            for (const auto pair : *ptr)
            {
                for (const auto& sec_pair : *pair.second)
                {
                    res->add_key_and_uploadid(pair.first, sec_pair);
                }
            }
        }

        return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_delete_objects (http::http_request& req)
    {

        std::unique_ptr<http::model::delete_objects_response> res;

        res = std::make_unique<http::model::delete_objects_response>(req);

        rest::utils::parser::xml_parser parsed_xml;
        pugi::xpath_node_set object_nodes_set;

        try
        {
            if (!parsed_xml.parse(req.get_body()))
                throw std::runtime_error("");

            object_nodes_set = parsed_xml.get_nodes_from_path("/Delete/Object");
            if (object_nodes_set.empty())
                throw std::runtime_error("");
        }
        catch(const std::exception& e)
        {
            throw http::model::custom_error_response_exception(b_http::status::bad_request, http::model::error::type::malformed_xml);
        }

        auto bucket_id = req.get_URI().get_bucket_id();
        for (const auto& objectNode : object_nodes_set)
        {
            auto key = objectNode.node().child("Key").child_value();

            try
            {
                auto m_dir = m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size()))->acquire_messenger();
                directory_message dir_req;
                dir_req.bucket_id = bucket_id;
                dir_req.object_key = std::make_unique <std::string> (key);

                co_await m_dir.get().send_directory_message (DIR_DELETE_OBJ_REQ, dir_req);

                co_await m_dir.get().recv_header();

                res->add_deleted_keys(key);
            }
            catch (const error_exception& e)
            {
                LOG_ERROR() << "Failed to delete the bucket " << bucket_id << " to the directory: " << e;
                res->add_failed_keys({key, e.error().code()});
            }
        }

        co_return std::move(res);
    }

    [[nodiscard]] client& get_recovery_director () const {
        return *m_directory_nodes.at(0);
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

    coro <dedupe_response> integrate_data (const std::list <std::string_view>& data_pieces) {

        size_t total_size = 0;
        std::map <size_t, std::string_view> offset_pieces;
        for (const auto& dp: data_pieces) {
            offset_pieces.emplace_hint(offset_pieces.cend(), total_size, dp);
            total_size += dp.size();
        }
        const auto part_size = static_cast <size_t> (std::ceil (static_cast <double> (total_size) / static_cast <double> (m_dedupe_nodes.size())));

        auto upload_to_dd = [&] (client::acquired_messenger m, int i) -> coro <dedupe_response> {
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
            co_return co_await m.get().recv_dedupe_response(h_dedup);
        };

        auto responses = co_await broadcast_gather_custom <dedupe_response> (*m_ioc, m_dedupe_nodes, upload_to_dd);

        dedupe_response resp {.effective_size = 0};

        for (const auto& r: responses) {
            resp.effective_size += r.second.effective_size;
            resp.addr.append_address(r.second.addr);
        }
        co_return resp;
    }

    std::atomic <size_t> m_directory_node_index {};
    entry_node_config m_entry_node_config {};

    std::shared_ptr <boost::asio::io_context> m_ioc;
    std::vector <std::shared_ptr <client>>& m_dedupe_nodes;
    std::vector <std::shared_ptr <client>>& m_directory_nodes;

    prometheus::Family<prometheus::Counter> &m_req_counters;
    prometheus::Counter &m_reqs_create_bucket;
    prometheus::Counter &m_reqs_get_bucket;
    prometheus::Counter &m_reqs_list_buckets;
    prometheus::Counter &m_reqs_delete_bucket;
    prometheus::Counter &m_reqs_delete_objects;
    prometheus::Counter &m_reqs_put_object;
    prometheus::Counter &m_reqs_get_object;
    prometheus::Counter &m_reqs_delete_object;
    prometheus::Counter &m_reqs_list_objects_v2;
    prometheus::Counter &m_reqs_list_objects;
    prometheus::Counter &m_reqs_get_object_attributes;
    prometheus::Counter &m_reqs_init_multipart_upload;
    prometheus::Counter &m_reqs_multiplart_upload;
    prometheus::Counter &m_reqs_complete_multipart_upload;
    prometheus::Counter &m_reqs_abort_multipart_upload;
    prometheus::Counter &m_reqs_list_multi_part_uploads;
    prometheus::Counter &m_reqs_invalid;
};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_REST_HANDLER_H
