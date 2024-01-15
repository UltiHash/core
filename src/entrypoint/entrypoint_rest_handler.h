//
// Created by masi on 8/29/23.
//

#ifndef CORE_ENTRY_NODE_REST_HANDLER_H
#define CORE_ENTRY_NODE_REST_HANDLER_H

#include <iostream>
#include <memory>
#include "common/utils/metrics_handler.h"
#include "common/utils/log.h"
#include "common/network/client.h"
#include "common/utils/worker_utils.h"

// HTTP
#include "rest/http/http_request.h"
#include "rest/http/http_response.h"
#include "rest/http/models/put_object_response.h"
#include "rest/http/models/get_object_response.h"
#include "rest/http/models/create_bucket_response.h"
#include "rest/http/models/init_multi_part_upload_response.h"
#include "rest/http/models/multi_part_upload_response.h"
#include "rest/http/models/complete_multi_part_upload_response.h"
#include "rest/http/models/abort_multi_part_upload_response.h"
#include "rest/http/models/list_buckets_response.h"
#include "rest/http/models/get_object_attributes_response.h"
#include "rest/http/models/list_objectsv2_response.h"
#include "rest/http/models/list_objects_response.h"
#include "rest/http/models/delete_bucket_response.h"
#include "rest/http/models/delete_object_response.h"
#include "rest/http/models/delete_objects_response.h"
#include "rest/http/models/list_multi_part_uploads_response.h"
#include "rest/http/models/get_bucket_response.h"
#include "rest/http/models/delete_objects_request.h"
#include "rest/http/models/custom_error_response_exception.h"

// UTILS
#include "rest/utils/parser/xml_parser.h"
#include "rest/utils/string/string_utils.h"
#include "rest/utils/hashing/hash.h"
#include "rest/utils/state/server_state.h"

namespace uh::cluster {

    namespace http = rest::http;
    namespace beast = boost::beast;         // from <boost/beast.hpp>
    namespace b_http = beast::http;           // from <boost/beast/http.hpp>

class entrypoint_rest_handler : protected metrics_handler {
public:

    entrypoint_rest_handler (std::shared_ptr <boost::asio::io_context> ioc,
                             std::vector <std::shared_ptr <client>>& dedupe_nodes,
                             std::vector <std::shared_ptr <client>>& directory_nodes,
                             entrypoint_config config,
                             std::shared_ptr <boost::asio::thread_pool> workers):
            metrics_handler(config.rest_server_conf),
            m_ioc (std::move (ioc)),
            m_workers (std::move (workers)),
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

    coro < std::unique_ptr<http::http_response> > handle (http::http_request& req, rest::utils::server_state& state)
    {

        std::unique_ptr<http::http_response> res;

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
                res = co_await handle_init_mp_upload(req, state);
                break;
            case http::http_request_type::MULTIPART_UPLOAD:
                m_reqs_multiplart_upload.Increment();
                res = co_await handle_mp_upload(req, state);
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
                res = co_await handle_list_mp_uploads(req, state);
                break;
            default:
                m_reqs_invalid.Increment();
                throw std::runtime_error("request not supported by the backend yet.");
        }

        co_return std::move(res);
    }

    coro <std::unique_ptr<http::http_response>> handle_create_bucket (const http::http_request& req)
    {

        std::unique_ptr<http::model::create_object_response> res = std::make_unique<http::model::create_object_response>(req);
        auto bucket_id = req.get_URI().get_bucket_id();

        try
        {
            auto func = [&bucket_id] (const auto& bucket, client::acquired_messenger m, long id) -> coro <void> {
                directory_message dir_req {.bucket_id = bucket_id};
                co_await m.get().send_directory_message(DIR_PUT_BUCKET_REQ, dir_req);
                co_await m.get().recv_header();
            };
            co_await worker_utils::broadcast_from_io_thread_in_io_threads (m_directory_nodes, *m_ioc, *m_workers, std::bind_front (func, std::cref (bucket_id)));
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

            auto func = [] (const http::http_request& req, client::acquired_messenger m, long id) -> coro <void> {
                directory_message dir_req;
                dir_req.bucket_id = req.get_URI().get_bucket_id();
                co_await m.get().send_directory_message (DIR_DELETE_BUCKET_REQ, dir_req);
                co_await m.get().recv_header();
            };
            co_await worker_utils::broadcast_from_io_thread_in_io_threads (m_directory_nodes, *m_ioc, *m_workers, std::bind_front(func, std::cref (req)));
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

        std::unique_ptr<http::model::get_bucket_response> res = std::make_unique <http::model::get_bucket_response> (req);
        auto req_bucket_id = req.get_URI().get_bucket_id();

        try {

            auto func = [] (std::unique_ptr<http::model::get_bucket_response>& res, const std::string& req_bucket_id, client::acquired_messenger m) -> coro <void> {
                co_await m.get().send(DIR_LIST_BUCKET_REQ, {});
                const auto h = co_await m.get().recv_header();
                const auto list_buckets_res = co_await m.get().recv_directory_list_entities_message(h);
                for (const auto& bucket: list_buckets_res.entities) {
                    if (bucket == req_bucket_id) {
                        res->add_bucket(bucket);
                        break;
                    }
                }

                if (res->get_bucket().empty()) {
                    throw error_exception(error::bucket_not_found);
                }
            };

            co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                       *m_ioc,
                                                                                       *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                       std::bind_front (func, std::ref (res), std::cref (req_bucket_id)));

        }
        catch (const error_exception& e)
        {
            LOG_ERROR() << "Failed to get bucket `" << req_bucket_id << "`: " << e;
            switch (*e.error()) {
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

        std::unique_ptr<http::model::list_buckets_response> res = std::make_unique <http::model::list_buckets_response> (req);

        auto func = [] (std::unique_ptr<http::model::list_buckets_response>& res, client::acquired_messenger m) -> coro <void> {
            co_await m.get().send(DIR_LIST_BUCKET_REQ, {});
            const auto h = co_await m.get().recv_header();
            auto list_buckets_res = co_await m.get().recv_directory_list_entities_message(h);
            for (const auto& bucket: list_buckets_res.entities)
            {
                res->add_bucket(bucket);
            }
        };

        co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                   *m_ioc,
                                                                                   *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                   std::bind_front (func, std::ref (res)));

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
            if (body_size > 0) [[likely]] {
                std::list <std::string_view> data {req.get_body()};
                resp = co_await integrate_data(data);
            }


            const directory_message dir_req {
                    .bucket_id = req.get_URI().get_bucket_id(),
                    .object_key = std::make_unique <std::string> (req.get_URI().get_object_key()),
                    .addr = std::make_unique <address> (std::move (resp.addr)),
            };

            auto func = [] (const directory_message& dir_req, client::acquired_messenger m, long id) -> coro <void> {
                co_await m.get().send_directory_message (DIR_PUT_OBJ_REQ, dir_req);
                co_await m.get().recv_header();
            };
            co_await worker_utils::broadcast_from_io_thread_in_io_threads (m_directory_nodes, *m_ioc, *m_workers, std::bind_front(func, std::cref (dir_req)));


            auto effective_size = static_cast <double> (resp.effective_size) / static_cast <double> (1024ul * 1024ul);
            auto space_saving = 1.0 - static_cast <double> (resp.effective_size) / static_cast <double> (body_size);
            const auto stop = std::chrono::steady_clock::now ();
            const std::chrono::duration <double> duration = stop - start;
            const auto bandwidth = size_mb / duration.count();

            LOG_INFO() << "original size " << size_mb << " MB";
            LOG_INFO() << "effective size " << effective_size << " MB";
            LOG_INFO() << "space saving " << space_saving;
            LOG_INFO() << "integration duration " << duration.count() << " s";
            LOG_INFO() << "integration bandwidth " << bandwidth << " MB/s";

            rest::utils::hashing::MD5 md5;
            res->set_etag(md5.calculateMD5(req.get_body()));
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
            std::string buffer;

            auto func = [] (std::string& buffer, const http::http_request& req, client::acquired_messenger m) -> coro <void> {
                directory_message dir_req;
                dir_req.bucket_id = req.get_URI().get_bucket_id();
                dir_req.object_key = std::make_unique <std::string> (req.get_URI().get_object_key());

                co_await m.get().send_directory_message (DIR_GET_OBJ_REQ, dir_req);
                const auto h_dir = co_await m.get().recv_header();

                buffer.resize (h_dir.size);
                m.get().register_read_buffer(buffer);
                co_await m.get().recv_buffers(h_dir);

            };
            co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                       *m_ioc,
                                                                                       *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                       std::bind_front (func, std::ref (buffer), std::cref (req)));

            const auto stop = std::chrono::steady_clock::now ();
            const std::chrono::duration <double> duration = stop - start;
            const auto size = static_cast <double> (buffer.size ()) / static_cast <double> (1024ul * 1024ul);
            const auto bandwidth = size / duration.count();
            LOG_INFO() << "retrieval duration " << duration.count() << " s";
            LOG_INFO() << "retrieval bandwidth " << bandwidth << " MB/s";

            res = std::make_unique<http::model::get_object_response>(req);
            res->set_body(std::move (buffer));
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

            auto func = [] (std::unique_ptr<http::model::list_objectsv2_response>& res, const http::http_request& req, client::acquired_messenger m) -> coro <void> {
                res = std::make_unique<http::model::list_objectsv2_response>(req);
                directory_message dir_req;
                dir_req.bucket_id = req.get_URI().get_bucket_id();

                co_await m.get().send_directory_message (DIR_LIST_OBJ_REQ, dir_req);
                const auto h_dir = co_await m.get().recv_header();

                unique_buffer <char> buffer (h_dir.size);
                directory_lst_entities_message list_objects_res;

                list_objects_res = co_await m.get().recv_directory_list_entities_message(h_dir);

                for (const auto& content : list_objects_res.entities)
                {
                    res->add_content(content);
                }
            };

            co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                       *m_ioc,
                                                                                       *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                       std::bind_front (func, std::ref (res), std::cref (req)));

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

        auto func = [] (std::unique_ptr<http::model::list_objects_response>& res, const http::http_request& req, client::acquired_messenger m) -> coro <void> {
            res = std::make_unique<http::model::list_objects_response>(req);

            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();

            co_await m.get().send_directory_message (DIR_LIST_OBJ_REQ, dir_req);
            const auto h_dir = co_await m.get().recv_header();

            unique_buffer <char> buffer (h_dir.size);
            directory_lst_entities_message list_objects_res;

            list_objects_res = co_await m.get().recv_directory_list_entities_message(h_dir);

            for (const auto& content : list_objects_res.entities) {
                res->add_content(content);
            }
            res->add_name(req.get_URI().get_bucket_id());

        };

        co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                   *m_ioc,
                                                                                   *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                   std::bind_front (func, std::ref (res), std::cref (req)));

        co_return std::move(res);
    }




      coro <std::unique_ptr<http::http_response>> handle_init_mp_upload (const http::http_request& req, rest::utils::server_state& state)
      {
          std::unique_ptr<http::model::init_multi_part_upload_response> res;
          try
          {
              res = std::make_unique<http::model::init_multi_part_upload_response>(req);

              co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                         *m_ioc,
                                                                                         *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                         [&res, &req] (client::acquired_messenger m) -> coro <void>
                  {
                      directory_message dir_req {.bucket_id = req.get_URI().get_bucket_id()};

                      co_await m.get().send_directory_message (DIR_BUCKET_EXISTS, dir_req);
                      co_await m.get().recv_header();

                      res->set_upload_id(req.get_eTag());
                  });

          }

          catch (const error_exception& e)
          {
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

    coro <std::unique_ptr<http::http_response>> handle_mp_upload (const http::http_request& req, rest::utils::server_state& state) {
        if (req.get_body_size() > 0) [[likely]] {
            std::list <std::string_view> data {req.get_body()};
            const auto resp = co_await integrate_data(data);
            auto func = [](rest::utils::server_state& state, const http::http_request& req, const auto& resp) {
                state.m_uploads.append_upload_part_info(req.get_URI().get_query_parameters().at("uploadId"),
                                                        std::stoi (req.get_URI().get_query_parameters().at("partNumber")), resp,
                                                        req.get_body());
            };
            co_await worker_utils::post_in_workers (*m_workers, *m_ioc, std::bind_front(func, std::ref (state), std::cref (req), std::cref (resp)));
        }
        std::unique_ptr<http::model::multi_part_upload_response> res = std::make_unique<http::model::multi_part_upload_response>(req);
        co_return std::move(res);
    }


    coro<std::unique_ptr<http::http_response>> handle_complete_mp_upload (http::http_request& req, rest::utils::server_state& state)
    {
        auto res = std::make_unique<http::model::complete_multi_part_upload_response>(req);

        auto up_info = state.m_uploads.get_upload_info(req.get_URI().get_query_parameters().at("uploadId"));
        const directory_message dir_req {
                .bucket_id = req.get_URI().get_bucket_id(),
                .object_key = std::make_unique <std::string> (req.get_URI().get_object_key()),
                .addr = std::make_unique <address> (std::move (up_info->get_address())),
        };

        auto func_dir = [] (const directory_message& dir_req, client::acquired_messenger m, long id) -> coro <void> {
            co_await m.get().send_directory_message (DIR_PUT_OBJ_REQ, dir_req);
            co_await m.get().recv_header();
        };

        co_await worker_utils::broadcast_from_io_thread_in_io_threads (m_directory_nodes,
                                                                       *m_ioc,
                                                                       *m_workers,
                                                                       std::bind_front (func_dir, std::cref (dir_req)));

        const auto size_mb = static_cast <double> (up_info->data_size) / static_cast <double> (1024ul * 1024ul);
        auto effective_size = static_cast <double> (up_info->effective_size) / static_cast <double> (1024ul * 1024ul);
        auto space_saving = 1.0 - effective_size / size_mb;
        const auto stop = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now ().time_since_epoch()).count();
        const auto dur_ms = stop - up_info->upload_init_time;

        const double dur_s = static_cast <double> (dur_ms) / 1000.0;
        const auto bandwidth = size_mb / dur_s;

        LOG_INFO() << "upload size: " << req.get_body_size();
        LOG_INFO() << "original size " << size_mb << " MB";
        LOG_INFO() << "effective size " << effective_size << " MB";
        LOG_INFO() << "space saving " << space_saving;
        LOG_INFO() << "integration duration " << dur_s << " s";
        LOG_INFO() << "integration bandwidth " << bandwidth << " MB/s";

        rest::utils::hashing::MD5 md5;
        res->set_etag(md5.calculateMD5(req.get_body()));
        res->set_size(size_mb);
        res->set_effective_size(effective_size);
        res->set_space_savings(space_saving);
        res->set_bandwidth(bandwidth);

        co_return std::move(res);
    }


    std::unique_ptr<http::http_response> handle_abort_mp_upload (const http::http_request& req)
    {
        std::unique_ptr<http::model::abort_multi_part_upload_response> res = std::make_unique<http::model::abort_multi_part_upload_response>(req);
        return std::move(res);
    }

    coro<std::unique_ptr<http::http_response>> handle_delete_object (const http::http_request& req)
    {

        std::unique_ptr<http::model::delete_object_response> res = std::make_unique<http::model::delete_object_response>(req);

        try
        {
            auto func = [] (const http::http_request& req, client::acquired_messenger m) -> coro <void> {
                directory_message dir_req {.bucket_id = req.get_URI().get_bucket_id(), .object_key = std::make_unique <std::string> (req.get_URI().get_object_key())};
                co_await m.get().send_directory_message (DIR_DELETE_OBJ_REQ, dir_req);
                co_await m.get().recv_header();
            };

            co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                       *m_ioc,
                                                                                       *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                       std::bind_front (func, std::cref (req)));

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


    coro <std::unique_ptr<http::http_response>> handle_list_mp_uploads (const http::http_request& req, rest::utils::server_state& state)
    {

        std::unique_ptr<http::model::list_multi_part_uploads_response> res = std::make_unique<http::model::list_multi_part_uploads_response>(req);

        auto func = [] (std::unique_ptr<http::model::list_multi_part_uploads_response>& res, rest::utils::server_state& state, const http::http_request& req) {
            auto bucket_name = req.get_URI().get_bucket_id();
            auto multipart_map = state.m_uploads.list_multipart_uploads(bucket_name);

            if (multipart_map.empty())
            {
                throw http::model::custom_error_response_exception(b_http::status::not_found, http::model::error::bucket_not_found);
            }
            else
            {
                for (const auto& pair : multipart_map)
                {
                    res->add_key_and_uploadid(pair.first, pair.second);
                }
            }
        };

        co_await worker_utils::post_in_workers (*m_workers, *m_ioc, std::bind_front (func, std::ref (res), std::ref (state), std::cref (req)));

        co_return std::move(res);
    }


    coro <std::unique_ptr<http::http_response>> handle_delete_objects (http::http_request& req)
    {

        std::unique_ptr<http::model::delete_objects_response> res;

        res = std::make_unique<http::model::delete_objects_response>(req);

        pugi::xpath_node_set object_nodes_set;

        auto func = [] (http::http_request& req, pugi::xpath_node_set& object_nodes_set) {
            rest::utils::parser::xml_parser parsed_xml;
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
        };

        co_await worker_utils::post_in_workers (*m_workers, *m_ioc, std::bind_front(func, std::ref (req), std::ref (object_nodes_set)));


        auto bucket_id = req.get_URI().get_bucket_id();
        for (const auto& objectNode : object_nodes_set)
        {
            auto key = objectNode.node().child("Key").child_value();

            try
            {

                auto func2 = [] (const char* key, const std::string& bucket_id, std::unique_ptr<http::model::delete_objects_response>& res, client::acquired_messenger m) -> coro <void> {
                    directory_message dir_req;
                    dir_req.bucket_id = bucket_id;
                    dir_req.object_key = std::make_unique <std::string> (key);

                    co_await m.get().send_directory_message (DIR_DELETE_OBJ_REQ, dir_req);

                    co_await m.get().recv_header();

                    res->add_deleted_keys(key);
                };
                co_await worker_utils::io_thread_acquire_messenger_and_post_in_io_threads (*m_workers,
                                                                                           *m_ioc,
                                                                                           *m_directory_nodes.at(get_round_robin_index(m_directory_node_index, m_directory_nodes.size())),
                                                                                           std::bind_front(func2, key, std::cref (bucket_id), std::ref (res)));

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

            std::vector <dedupe_response> responses (m_dedupe_nodes.size());

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

            co_await worker_utils::broadcast_from_io_thread_in_io_threads (m_dedupe_nodes,
                                                                           *m_ioc,
                                                                           *m_workers,
                                                                           std::bind_front (func, part_size, std::cref (offset_pieces), std::ref (responses)));


            dedupe_response resp {.effective_size = 0};

            for (const auto& r: responses) {
                resp.effective_size += r.effective_size;
                resp.addr.append_address(r.addr);
            }
            co_return resp;
        }

        std::atomic <size_t> m_directory_node_index {};

        std::shared_ptr <boost::asio::io_context> m_ioc;
        std::shared_ptr <boost::asio::thread_pool> m_workers;

        std::vector <std::shared_ptr <client>>& m_dedupe_nodes;
        std::vector <std::shared_ptr <client>>& m_directory_nodes;
        entrypoint_config m_entry_node_config {};


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
