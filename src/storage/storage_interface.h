
#ifndef UH_CLUSTER_STORAGE_INTERFACE_H
#define UH_CLUSTER_STORAGE_INTERFACE_H

#include "common/network/messenger_core.h"
#include "common/registry/services.h"
#include "common/types/address.h"
#include "common/utils/pointer_traits.h"
#include "data_store.h"
#include <boost/asio/thread_pool.hpp>
#include <span>
#include <utility>

namespace uh::cluster {
struct storage_interface {
    virtual coro <address> write (std::span <char>) = 0;
    virtual coro <void> read_fragment (char* buffer, const fragment& f) = 0;
    virtual coro <void> read_address (char* buffer, const address& addr, const std::vector<size_t>& offsets) = 0;
    virtual coro <void> sync (const address& addr) = 0;
    virtual coro <size_t> get_used_space () = 0;
    virtual coro <size_t> get_free_space () = 0;
    virtual ~storage_interface() = default;
};


struct remote_storage: public storage_interface {

    explicit remote_storage (coro_client storage_service):
          m_storage_service (std::move(storage_service)){}

    coro <address> write (std::span <char> data) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send(STORAGE_WRITE_REQ, data);
        const auto message_header = co_await m.get().recv_header();
        co_return co_await m.get().recv_address(message_header);
    }

    coro <void> read_fragment (char* buffer, const fragment& frag) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send_fragment(STORAGE_READ_FRAGMENT_REQ, frag);
        const auto h = co_await m.get().recv_header();
        if (h.size != frag.size) [[unlikely]] {
            throw std::runtime_error("Incomplete fragment");
        }
        m.get().register_read_buffer(buffer, frag.size);
        co_await m.get().recv_buffers(h);
    }

    coro <void> read_address (char* buffer, const address& addr, const std::vector<size_t>& offsets) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send_address(STORAGE_READ_ADDRESS_REQ, addr);
        const auto h = co_await m.get().recv_header();
        m.get().reserve_read_buffers(addr.size());
        for (size_t i = 0; i < addr.size(); ++i) {
            m.get().register_read_buffer(buffer + offsets.at(i),
                                         addr.sizes[i]);
        }
        co_await m.get().recv_buffers(h);
    }

    coro <void> sync (const address& addr) override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send_address(STORAGE_SYNC_REQ, addr);
        co_await m.get().recv_header();
    }

    coro <size_t> get_used_space () override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send(STORAGE_USED_REQ, {});
        const auto message_header = co_await m.get().recv_header();
        co_return co_await m.get().recv_primitive<size_t>(message_header);
    }

    coro <size_t> get_free_space () override {
        auto m = co_await m_storage_service.acquire_messenger();
        co_await m.get().send(STORAGE_AVAILABLE_REQ, {});
        const auto message_header = co_await m.get().recv_header();
        co_return co_await m.get().recv_primitive<size_t>(message_header);
    }

private:
    coro_client m_storage_service;
};

struct local_storage: public storage_interface {
    coro <address> write (std::span <char> data) override {
        const size_t part = std::ceil ((double) data.size() / (double)m_data_stores.size());

        address total_addr;
        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            const auto part_size = std::min(data.size() - i * part, part);
            auto addr = m_data_stores[i]->register_write(data.subspan(i * part, part_size));
            total_addr.append_address(addr);
            boost::asio::post (m_threads, [addr = std::move (addr), i, this] () {m_data_stores[i]->perform_write(addr);});
        }

        co_return total_addr;
    }

    coro <void> read_fragment (char* buffer, const fragment& f) override {
        get_data_store(f.pointer).read(buffer, f.pointer, f.size);
        co_return;
    }

    coro <void> read_address (char* buffer, const address& addr, const std::vector<size_t>& offsets) override {

        for (size_t i = 0; i < addr.size(); i++) {
            const auto frag = addr.get_fragment(i);
            if (get_data_store(frag.pointer).read(buffer + offsets[i], frag.pointer,
                                                  frag.size) != frag.size) [[unlikely]] {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
        }
        co_return;
    }

    coro <void> sync (const address& addr) override {

        std::vector <address> ds_addresses (m_data_stores.size());
        for (size_t i = 0; i < addr.size(); i++) {
            const auto f = addr.get_fragment(i);
            const auto id = pointer_traits::get_data_store_id(f.pointer);
            ds_addresses.at(id).push_fragment(f);
        }

        std::vector <std::future <void>> futures;
        futures.reserve (m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {

            auto p = std::make_shared<std::promise <void>>();
            boost::asio::post (m_threads, [i, this, p, &ds_addresses] () {
                try {
                    m_data_stores[i]->wait_for_ongoing_writes(ds_addresses[i]);
                    m_data_stores[i]->sync();
                    p->set_value();
                }
                catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());

        }
        for(auto& f: futures){
            f.get();
        }
        co_return;
    }

    coro <size_t> get_used_space () override {
        size_t used = 0;
        for (const auto& ds: m_data_stores) {
            used += ds->get_used_space();
        }
        co_return used;
    }

    coro <size_t> get_free_space () override {
        size_t free = 0;
        for (const auto& ds: m_data_stores) {
            free += ds->get_available_space();
        }
        co_return free;
    }

private:
    std::vector <std::unique_ptr <data_store>> m_data_stores;
    boost::asio::thread_pool m_threads;

    data_store& get_data_store (const uint128_t& pointer) {
        return *m_data_stores[pointer_traits::get_data_store_id(pointer)];
    }

};



struct storage_services {

    storage_services (boost::asio::io_context& ioc, const int connection_count,
             etcd::SyncClient& etcd_client)
        : m_ioc(ioc),
          m_connection_count(connection_count),
          m_etcd_client(etcd_client),
          m_watcher(
              m_etcd_client,
              etcd_services_announced_key_prefix + get_service_string(STORAGE_SERVICE),
              [this](const etcd::Response& response) {
                  return handle_state_changes(response);
              },
              true),
          m_robin_index(m_clients.end()) {
        auto path = etcd_services_announced_key_prefix + get_service_string(STORAGE_SERVICE);

        auto resp = wait_for_success(
            ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
            [this, &path]() { return m_etcd_client.ls(path); });
        for (const auto& key : resp.keys()) {
            add(key);
        }
    }

    ~storage_services() { m_watcher.Cancel(); }

    template <typename key> std::shared_ptr<client> get(const uint128_t& pointer) const {
        std::shared_ptr<client> cl;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_cv.wait_for(
                lk, std::chrono::seconds(m_timeout_s), [this, &pointer, &cl]() {
                    try {
                        auto id = pointer_traits::get_service_id (pointer);
                        return m_clients.at(id);
                    } catch (const std::out_of_range& range_exception) {
                        return nullptr;
                    }
                })) {
        } else
            throw std::runtime_error("timeout waiting for client");

        return cl;
    }

    std::shared_ptr<storage_interface> get(uint32_t id) const {
        std::shared_ptr<storage_interface> cl;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                          [this, &id, &cl]() {
                              auto it = m_clients.find(id);

                              if (it == m_clients.end())
                                  return false;

                              cl = it->second;
                              return true;
                          })) {
        } else
            throw std::runtime_error("timeout waiting for client");

        return cl;
    }

    std::shared_ptr<storage_interface> get() const {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                          [this]() { return !m_clients.empty(); })) {
        } else
            throw std::runtime_error("timeout waiting for client");

        if (m_robin_index == m_clients.end()) {
            m_robin_index = m_clients.begin();
        }

        auto rv = m_robin_index->second;
        ++m_robin_index;

        return rv;
    }

    std::vector<std::shared_ptr<storage_interface>> get_clients() const {
        std::vector<std::shared_ptr<storage_interface>> clients_list;

        std::shared_lock<std::shared_mutex> lk(m_mutex);
        clients_list.reserve(m_clients.size());
        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }
private:
    struct service_endpoint {
        uint32_t id{};
        std::string host{};
        std::uint16_t port{};
    };

    void handle_state_changes(const etcd::Response& response) {
        LOG_DEBUG() << "action: " << response.action()
                    << ", key: " << response.value().key()
                    << ", value: " << response.value().as_string();

        try {
            const auto& etcd_path = response.value().key();
            const auto etcd_action = get_etcd_action_enum(response.action());

            switch (etcd_action) {
            case etcd_action::create:
                add(etcd_path);
                break;

            case etcd_action::erase:
                remove(etcd_path);
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error while handling service state change: "
                       << e.what();
        }
    }

    service_endpoint extract(const std::string& path) {

        const auto id = std::filesystem::path(path).filename().string();
        service_endpoint service_endpoint;
        service_endpoint.id = std::stoul(id);

        const std::string attributes_prefix(
            etcd_services_attributes_key_prefix + get_service_string(STORAGE_SERVICE) + '/' +
            id + '/');

        const auto attributes = wait_for_success(
            ETCD_TIMEOUT, ETCD_RETRY_INTERVAL, [this, &attributes_prefix]() {
                return m_etcd_client.ls(attributes_prefix);
            });

        std::optional<std::string> host;
        std::optional<uint16_t> port;
        for (size_t i = 0; i < attributes.keys().size(); i++) {
            const auto attribute_name =
                std::filesystem::path(attributes.key(i)).filename().string();

            if (attribute_name == get_config_string(CFG_ENDPOINT_HOST)) {
                host = attributes.value(i).as_string();
            } else if (attribute_name == get_config_string(CFG_ENDPOINT_PORT)) {
                port = std::stoul(attributes.value(i).as_string());
            }
        }

        if (!host || !port) {
            throw std::runtime_error("client not available");
        }

        service_endpoint.port = *port;
        service_endpoint.host = *host;
        return service_endpoint;
    }

    void add(const std::string& path) {
        const auto service_endpoint = extract(path);

        LOG_DEBUG() << "add callback for service " << get_service_string(STORAGE_SERVICE)
                    << ": " << service_endpoint.id
                    << " called. host: " << service_endpoint.host
                    << " port: " << service_endpoint.port;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_clients.contains(service_endpoint.id)) [[unlikely]]
            return;

        auto cl =
            std::make_shared<remote_storage>(coro_client {m_ioc, service_endpoint.host,
                                             service_endpoint.port, m_connection_count});

        m_clients.emplace(service_endpoint.id, cl);
        lk.unlock();
        m_cv.notify_one();
    }

    void remove(const std::string& path) {
        const auto id =
            std::stoul(std::filesystem::path(path).filename().string());

        LOG_DEBUG() << "remove callback for service " << get_service_string(STORAGE_SERVICE)
                    << ": " << id << " called. ";

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        auto it = m_clients.find(id);
        if (it == m_clients.end()) {
            return;
        }

        if (it == m_robin_index) {
            m_robin_index = m_clients.erase(it);
        } else {
            m_clients.erase(it);
        }

    }

    boost::asio::io_context& m_ioc;
    const int m_connection_count;
    etcd::SyncClient& m_etcd_client;
    etcd::Watcher m_watcher;

    mutable std::shared_mutex m_mutex;
    mutable std::condition_variable_any m_cv;
    std::map<uint32_t, std::shared_ptr<storage_interface>> m_clients;
    mutable decltype(m_clients)::const_iterator m_robin_index;
    static constexpr std::size_t m_timeout_s = 10;


};
}

#endif // UH_CLUSTER_STORAGE_INTERFACE_H
