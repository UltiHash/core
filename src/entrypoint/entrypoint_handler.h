#ifndef ENTRYPOINT_HANDLER_H
#define ENTRYPOINT_HANDLER_H

#include "common/registry/services.h"
#include "common/utils/error.h"
#include "common/utils/protocol_handler.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/http_response.h"
#include "entrypoint/rest/http/models/abort_multi_part_upload_response.h"
#include "entrypoint/rest/http/models/complete_multi_part_upload_response.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"
#include "entrypoint/rest/http/models/list_multi_part_uploads_response.h"
#include "entrypoint/rest/http/models/list_objects_response.h"
#include "entrypoint/rest/http/models/multi_part_upload_response.h"
#include "entrypoint/rest/utils/parser/s3_parser.h"
#include "entrypoint/rest/utils/parser/xml_parser.h"
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>

// REFACTORED
#include "entrypoint/requests/init_multipart_upload.h"
#include "requests/create_bucket.h"
#include "requests/delete_bucket.h"
#include "requests/delete_object.h"
#include "requests/delete_objects.h"
#include "requests/get_bucket.h"
#include "requests/get_object.h"
#include "requests/list_buckets.h"
#include "requests/list_objects_v2.h"
#include "requests/put_object.h"

namespace uh::cluster {

template <typename... RequestTypes>
class entrypoint_handler : public protocol_handler {
  public:
    explicit entrypoint_handler(entrypoint_state& state,
                                RequestTypes&&... request_types)
        : m_state(state), m_ioc(state.ioc), m_workers(state.workers),
          m_dedupe_services(state.dedupe_services),
          m_directory_services(state.directory_services),
          m_req_types(request_types...) {}

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        LOG_INFO() << "connection from: " << s.remote_endpoint();

        try {

            for (;;) {

                boost::beast::flat_buffer buffer;

                boost::beast::http::request_parser<
                    boost::beast::http::empty_body>
                    received_request;
                received_request.body_limit(
                    (std::numeric_limits<std::uint64_t>::max)());

                co_await boost::beast::http::async_read_header(
                    s, buffer, received_request, boost::asio::use_awaitable);
                LOG_DEBUG()
                    << "received request: " << received_request.get().base();

                bool fallback = false;
                try {
                    http_request req(received_request, s, buffer);
                    auto resp = co_await handle_request(req);
                    co_await boost::beast::http::async_write(
                        s, resp.get_prepared_response(),
                        boost::asio::use_awaitable);
                } catch (const command_unknown_exception&) {
                    fallback = true;
                }

                if (fallback) {
                    uh::cluster::rest::utils::parser::s3_parser s3_parser(
                        received_request, m_state.server_state);
                    auto s3_request = s3_parser.parse();

                    co_await s3_request->read_body(s, buffer);
                    s3_request->validate_request_specific_criteria();

                    auto s3_res = co_await handle_request(*s3_request,
                                                          m_state.server_state);
                    auto s3_res_specific_object =
                        s3_res->get_response_specific_object();
                    co_await boost::beast::http::async_write(
                        s, s3_res_specific_object, boost::asio::use_awaitable);
                    LOG_DEBUG()
                        << "sent response: " << s3_res_specific_object.base();
                }

                if (!received_request.keep_alive()) {
                    break;
                }
            }

        } catch (const boost::system::system_error& se) {
            if (se.code() != boost::beast::http::error::end_of_stream) {
                LOG_ERROR() << se.what();
                uh::cluster::rest::http::model::custom_error_response_exception
                    err(boost::beast::http::status::bad_request);
                boost::beast::http::write(s,
                                          err.get_response_specific_object());
                s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                s.close();
                throw;
            }
        } catch (
            uh::cluster::rest::http::model::custom_error_response_exception&
                res_exc) {
            LOG_ERROR() << res_exc.what();
            boost::beast::http::write(s,
                                      res_exc.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        } catch (const std::exception& e) {
            LOG_ERROR() << e.what();
            uh::cluster::rest::http::model::custom_error_response_exception err(
                boost::beast::http::status::internal_server_error);
            boost::beast::http::write(s, err.get_response_specific_object());
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            s.close();
            throw;
        }

        s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        s.close();
    }

    coro<http_response> handle_request(http_request& req) {
        return dispatch_unpack_tuple(
            req, m_req_types,
            std::make_index_sequence<
                std::tuple_size_v<decltype(m_req_types)>>());
    }

    template <std::size_t... Is>
    coro<http_response> dispatch_unpack_tuple(http_request& req,
                                              auto&& req_types,
                                              std::index_sequence<Is...>) {
        return dispatch_front(req, std::get<Is>(req_types)...);
    }

    template <typename command, typename... commands>
    coro<http_response> dispatch_front(http_request& req, command&& head,
                                       commands&&... tail) {
        if (head.can_handle(req)) {
            return head.handle(req);
        }
        return dispatch_front(req, std::forward<commands>(tail)...);
    }

    coro<http_response> dispatch_front(const http_request& req) {
        throw command_unknown_exception();
    }

    coro<std::unique_ptr<rest::http::http_response>>
    handle_request(rest::http::http_request& req,
                   rest::utils::server_state& state) {

        std::unique_ptr<rest::http::http_response> res;

        switch (req.get_request_name()) {
        case rest::http::http_request_type::LIST_OBJECTS:
            res = co_await handle_list_objects(req);
            break;
        case rest::http::http_request_type::MULTIPART_UPLOAD:
            res = co_await handle_mp_upload(req, state);
            break;
        case rest::http::http_request_type::COMPLETE_MULTIPART_UPLOAD:
            res = co_await handle_complete_mp_upload(req, state);
            break;
        case rest::http::http_request_type::ABORT_MULTIPART_UPLOAD:
            res = handle_abort_mp_upload(req);
            break;
        case rest::http::http_request_type::LIST_MULTI_PART_UPLOADS:
            res = co_await handle_list_mp_uploads(req, state);
            break;
        default:
            throw std::runtime_error(
                "request not supported by the backend yet.");
        }

        co_return std::move(res);
    }

    coro<std::unique_ptr<rest::http::http_response>>

    handle_list_objects(const rest::http::http_request& req) {
        std::unique_ptr<rest::http::model::list_objects_response> res;

        auto func =
            [](std::unique_ptr<rest::http::model::list_objects_response>& res,
               const rest::http::http_request& req,
               client::acquired_messenger m) -> coro<void> {
            res =
                std::make_unique<rest::http::model::list_objects_response>(req);

            directory_message dir_req;
            dir_req.bucket_id = req.get_URI().get_bucket_id();

            co_await m.get().send_directory_message(DIR_LIST_OBJ_REQ, dir_req);
            const auto h_dir = co_await m.get().recv_header();

            unique_buffer<char> buffer(h_dir.size);
            directory_lst_entities_message list_objects_res;

            list_objects_res =
                co_await m.get().recv_directory_list_entities_message(h_dir);

            for (const auto& content : list_objects_res.entities) {
                res->add_content(content);
            }
            res->add_name(req.get_URI().get_bucket_id());
        };

        co_await worker_utils::
            io_thread_acquire_messenger_and_post_in_io_threads(
                m_workers, m_ioc, m_directory_services.get(),
                std::bind_front(func, std::ref(res), std::cref(req)));

        co_return std::move(res);
    }

    coro<std::unique_ptr<rest::http::http_response>>
    handle_mp_upload(const rest::http::http_request& req,
                     rest::utils::server_state& state) {
        if (req.get_body_size() > 0) [[likely]] {
            std::list<std::string_view> data{req.get_body()};
            const auto resp = co_await integrate_data(data);
            auto func = [](rest::utils::server_state& state,
                           const rest::http::http_request& req,
                           const auto& resp) {
                state.m_uploads.append_upload_part_info(
                    req.get_URI().get_query_parameters().at("uploadId"),
                    std::stoi(
                        req.get_URI().get_query_parameters().at("partNumber")),
                    resp, req.get_body());
            };

            co_await worker_utils::post_in_workers(
                m_workers, m_ioc,
                std::bind_front(func, std::ref(state), std::cref(req),
                                std::cref(resp)));
        }
        std::unique_ptr<rest::http::model::multi_part_upload_response> res =
            std::make_unique<rest::http::model::multi_part_upload_response>(
                req);
        co_return std::move(res);
    }

    coro<std::unique_ptr<rest::http::http_response>>
    handle_complete_mp_upload(rest::http::http_request& req,
                              rest::utils::server_state& state) {
        auto res = std::make_unique<
            rest::http::model::complete_multi_part_upload_response>(req);

        auto up_info = state.m_uploads.get_upload_info(
            req.get_URI().get_query_parameters().at("uploadId"));
        const directory_message dir_req{
            .bucket_id = req.get_URI().get_bucket_id(),
            .object_key =
                std::make_unique<std::string>(req.get_URI().get_object_key()),
            .addr =
                std::make_unique<address>(up_info->generate_total_address()),
        };

        auto func_dir = [](const directory_message& dir_req,
                           client::acquired_messenger m,
                           long id) -> coro<void> {
            co_await m.get().send_directory_message(DIR_PUT_OBJ_REQ, dir_req);
            co_await m.get().recv_header();
        };

        co_await worker_utils::broadcast_from_io_thread_in_io_threads(
            m_directory_services.get_clients(), m_ioc, m_workers,
            std::bind_front(func_dir, std::cref(dir_req)));

        const auto size_mb = static_cast<double>(up_info->data_size) /
                             static_cast<double>(1024ul * 1024ul);
        auto effective_size = static_cast<double>(up_info->effective_size) /
                              static_cast<double>(1024ul * 1024ul);
        auto space_saving = 1.0 - effective_size / size_mb;
        const auto stop =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();
        const auto dur_ms = stop - up_info->upload_init_time;

        const double dur_s = static_cast<double>(dur_ms) / 1000.0;
        const auto bandwidth = size_mb / dur_s;

        LOG_DEBUG() << "upload size: " << req.get_body_size();
        LOG_DEBUG() << "original size " << size_mb << " MB";
        LOG_DEBUG() << "effective size " << effective_size << " MB";
        LOG_DEBUG() << "space saving " << space_saving;
        LOG_DEBUG() << "integration duration " << dur_s << " s";
        LOG_DEBUG() << "integration bandwidth " << bandwidth << " MB/s";

        rest::utils::hashing::MD5 md5;
        res->set_etag(md5.calculateMD5(req.get_body()));
        res->set_size(up_info->data_size);
        res->set_effective_size(up_info->effective_size);
        res->set_space_savings(space_saving);
        res->set_bandwidth(bandwidth);

        co_return std::move(res);
    }

    std::unique_ptr<rest::http::http_response>
    handle_abort_mp_upload(const rest::http::http_request& req) {
        std::unique_ptr<rest::http::model::abort_multi_part_upload_response>
            res = std::make_unique<
                rest::http::model::abort_multi_part_upload_response>(req);
        return std::move(res);
    }

    coro<std::unique_ptr<rest::http::http_response>>
    handle_list_mp_uploads(const rest::http::http_request& req,
                           rest::utils::server_state& state) {

        std::unique_ptr<rest::http::model::list_multi_part_uploads_response>
            res = std::make_unique<
                rest::http::model::list_multi_part_uploads_response>(req);

        auto func =
            [](std::unique_ptr<
                   rest::http::model::list_multi_part_uploads_response>& res,
               rest::utils::server_state& state,
               const rest::http::http_request& req) {
                auto bucket_name = req.get_URI().get_bucket_id();
                auto multipart_map =
                    state.m_uploads.list_multipart_uploads(bucket_name);

                if (multipart_map.empty()) {
                    throw rest::http::model::custom_error_response_exception(
                        boost::beast::http::status::not_found,
                        rest::http::model::error::bucket_not_found);
                } else {
                    for (const auto& pair : multipart_map) {
                        res->add_key_and_uploadid(pair.first, pair.second);
                    }
                }
            };

        co_await worker_utils::post_in_workers(
            m_workers, m_ioc,
            std::bind_front(func, std::ref(res), std::ref(state),
                            std::cref(req)));

        co_return std::move(res);
    }

  private:
    coro<dedupe_response>
    integrate_data(const std::list<std::string_view>& data_pieces) {

        size_t total_size = 0;
        std::map<size_t, std::string_view> offset_pieces;
        for (const auto& dp : data_pieces) {
            offset_pieces.emplace_hint(offset_pieces.cend(), total_size, dp);
            total_size += dp.size();
        }

        auto dedup_services = m_dedupe_services.get_clients();
        auto dedup_services_size = dedup_services.size();
        const auto part_size = static_cast<size_t>(
            std::ceil(static_cast<double>(total_size) /
                      static_cast<double>(dedup_services_size)));

        std::vector<dedupe_response> responses(dedup_services_size);

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
            dedup_services, m_ioc, m_workers,
            std::bind_front(func, part_size, std::cref(offset_pieces),
                            std::ref(responses)));

        dedupe_response resp{.effective_size = 0};

        for (const auto& r : responses) {
            resp.effective_size += r.effective_size;
            resp.addr.append_address(r.addr);
        }
        co_return resp;
    }

    entrypoint_state& m_state;
    boost::asio::io_context& m_ioc;
    boost::asio::thread_pool& m_workers;
    const services<DEDUPLICATOR_SERVICE>& m_dedupe_services;
    const services<DIRECTORY_SERVICE>& m_directory_services;
    std::tuple<RequestTypes...> m_req_types;
};

template <typename... RequestTypes>
auto define_entrypoint_handler(entrypoint_state& state,
                               RequestTypes&&... request_types) {
    return std::make_unique<entrypoint_handler<RequestTypes...>>(
        state, std::forward<RequestTypes>(request_types)...);
}

auto make_entrypoint_handler(entrypoint_state& state) {
    return define_entrypoint_handler(
        state, create_bucket(state), get_bucket(state), list_buckets(state),
        delete_bucket(state), put_object(state), get_object(state),
        list_objects_v2(state), delete_object(state), delete_objects(state), 
        init_multipart_upload(state));
}

} // end namespace uh::cluster

#endif // ENTRYPOINT_HANDLER_H
