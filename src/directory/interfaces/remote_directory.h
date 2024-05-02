
#ifndef UH_CLUSTER_REMOTE_DIRECTORY_H
#define UH_CLUSTER_REMOTE_DIRECTORY_H

#include "common/network/client.h"
#include "directory_interface.h"

namespace uh::cluster {

struct remote_directory : public directory_interface {

    struct remote_read_handle : public read_handle {
        acquired_messenger m_messenger;

        bool m_transfer_completed = false;
        remote_read_handle(acquired_messenger m)
            : m_messenger(std::move(m)) {}

        bool has_next() override { return !m_transfer_completed; }

        coro<std::string> next() override {
            auto h = co_await m_messenger->recv_header();
            auto [b_next, buf] =
                co_await m_messenger->recv_directory_get_object_chunk(h);
            m_transfer_completed = !b_next;
            co_return buf;
        }
    };

    explicit remote_directory(client directory_service)
        : m_directory_service(std::move(directory_service)) {}
    coro<void> put_object(const std::string& bucket,
                          const std::string& object_id,
                          const address& addr) override {
        const directory_message dir_req{
            .bucket_id = bucket,
            .object_key = std::make_unique<std::string>(object_id),
            .addr = std::make_unique<address>(addr),
        };
        auto m = co_await m_directory_service.acquire_messenger();
        co_await m->send_directory_message(DIRECTORY_OBJECT_PUT_REQ, dir_req);
        co_await m->recv_header();
    }
    coro<std::unique_ptr<read_handle>>
    get_object(const std::string& bucket,
               const std::string& object_id) override {
        auto m = co_await m_directory_service.acquire_messenger();

        directory_message dir_req;
        dir_req.bucket_id = bucket;
        dir_req.object_key = std::make_unique<std::string>(object_id);

        co_await m->send_directory_message(DIRECTORY_OBJECT_GET_REQ, dir_req);
        co_return std::make_unique<remote_read_handle>(std::move(m));
    }
    coro<void> put_bucket(const std::string& bucket) override {
        auto m = co_await m_directory_service.acquire_messenger();
        directory_message req{.bucket_id = bucket};
        co_await m->send_directory_message(DIRECTORY_BUCKET_PUT_REQ, req);
        co_await m->recv_header();
    }
    coro<void> bucket_exists(const std::string& bucket) override {
        auto m = co_await m_directory_service.acquire_messenger();
        directory_message req{.bucket_id = bucket};
        co_await m->send_directory_message(DIRECTORY_BUCKET_EXISTS_REQ, req);
        co_await m->recv_header();
    }
    coro<void> delete_bucket(const std::string& bucket) override {

        auto m = co_await m_directory_service.acquire_messenger();
        directory_message req;
        req.bucket_id = bucket;
        co_await m->send_directory_message(DIRECTORY_BUCKET_DELETE_REQ, req);
        co_await m->recv_header();
    }
    coro<void> delete_object(const std::string& bucket,
                             const std::string& object_id) override {
        directory_message dir_req{.bucket_id = bucket,
                                  .object_key =
                                      std::make_unique<std::string>(object_id)};
        auto m = co_await m_directory_service.acquire_messenger();

        co_await m->send_directory_message(DIRECTORY_OBJECT_DELETE_REQ,
                                           dir_req);
        co_await m->recv_header();
    }
    coro<std::vector<std::string>> list_buckets() override {
        auto m = co_await m_directory_service.acquire_messenger();

        co_await m->send(DIRECTORY_BUCKET_LIST_REQ, {});

        const auto h = co_await m->recv_header();
        auto result = co_await m->recv_directory_list_buckets_message(h);

        co_return std::move (result.entities);
    }
    coro<std::vector<object>>
    list_objects(const std::string& bucket,
                 const std::optional<std::string>& prefix,
                 const std::optional<std::string>& lower_bound) override {
        auto m = co_await m_directory_service.acquire_messenger();
        directory_message dir_req{.bucket_id = bucket};
        if (prefix) {
            dir_req.object_key_prefix = std::make_unique<std::string>(*prefix);
        }
        if (lower_bound) {
            dir_req.object_key_lower_bound =
                std::make_unique<std::string>(*lower_bound);
        }

        co_await m->send_directory_message(DIRECTORY_OBJECT_LIST_REQ, dir_req);

        auto h_dir = co_await m->recv_header();
        auto list_objs_res =
            co_await m->recv_directory_list_objects_message(h_dir);
        co_return std::move(list_objs_res.objects);
    }

private:
    client m_directory_service;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_REMOTE_DIRECTORY_H
