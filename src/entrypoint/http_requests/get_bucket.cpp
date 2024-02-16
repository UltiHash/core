#include "get_bucket.h"
#include "common/utils/worker_utils.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"

namespace uh::cluster {

get_bucket::get_bucket(const entrypoint_state& entry_state)
    : m_state(entry_state) {}

bool get_bucket::can_handle(const http_request& req) {

    if (req.get_method() == method::get) {

        if (const auto& uri = req.get_URI();
            !uri.get_bucket_id().empty() && uri.get_object_key().empty() &&
            uri.get_query_parameters().empty()) {
            return true;
        }
    }

    return false;
}

coro<http_response> get_bucket::handle(const http_request& req) const {
    auto req_bucket_id = req.get_URI().get_bucket_id();

    try {
        http_response res;
        std::string bucket_name;

        auto func = [](http_response& res, const std::string& req_bucket_id,
                       std::string& bucket_name,
                       client::acquired_messenger m) -> coro<void> {
            co_await m.get().send(DIR_LIST_BUCKET_REQ, {});
            const auto h = co_await m.get().recv_header();
            const auto list_buckets_res =
                co_await m.get().recv_directory_list_entities_message(h);

            for (const auto& bucket : list_buckets_res.entities) {
                if (bucket == req_bucket_id) {
                    bucket_name = std::move(bucket);
                    break;
                }
            }

            if (bucket_name.empty()) {
                throw error_exception(error::bucket_not_found);
            }
        };

        co_await worker_utils::
            io_thread_acquire_messenger_and_post_in_io_threads(
                m_state.workers, m_state.ioc, m_state.directory_services.get(),
                std::bind_front(func, std::ref(res), std::cref(req_bucket_id),
                                std::ref(bucket_name)));

        std::string bucket_xml = "<Bucket>" + bucket_name + "</Bucket>\n";
        res.set_body(std::string("<GetBucketResult>\n" + bucket_xml +
                                 "</GetBucketResult>"));

        co_return res;

    } catch (const error_exception& e) {
        LOG_ERROR() << "Failed to get bucket `" << req_bucket_id << "`: " << e;
        switch (*e.error()) {
        case error::bucket_not_found:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::not_found,
                rest::http::model::error::bucket_not_found);
        default:
            throw rest::http::model::custom_error_response_exception(
                boost::beast::http::status::internal_server_error);
        }
    }
}

} // namespace uh::cluster
