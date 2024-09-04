
#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H

#include "common/coroutines/coro_util.h"
#include "common/ec/ec_factory.h"
#include "common/ec/reedsolomon_c.h"
#include "common/etcd/service_discovery/storage_service_get_handler.h"
#include "common/utils/address_utils.h"
#include "common/utils/time_utils.h"

namespace uh::cluster {

struct storage_system_config {
    size_t data_nodes;
    size_t ec_nodes;
};

enum ec_status {
    empty,
    degraded,
    lost_data,
    recovering,
    healthy,
};

struct storage_group : public storage_interface {

    void insert(size_t id, size_t group_nid,
                const std::shared_ptr<storage_interface>& node) {
        m_nodes.at(group_nid) = node;
        m_getter.add_client(id, node);
        update_status();
    }

    void remove(size_t id, size_t group_nid) {
        m_getter.remove_client(id, m_nodes.at(group_nid));
        m_nodes.at(group_nid) = nullptr;
        update_status();
    }

    [[nodiscard]] bool is_healthy() const noexcept {
        return m_status == healthy;
    }

    [[nodiscard]] bool is_empty() const noexcept { return m_status == empty; }
    storage_group(boost::asio::io_context& ioc, size_t data_nodes,
                  size_t ec_nodes)
        : m_nodes(data_nodes + ec_nodes),
          m_ec_calc(ec_factory::create(data_nodes, ec_nodes)),
          m_ioc(ioc) {}

    coro<address> write(context& ctx, const std::string_view& data) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }
        auto encoded = m_ec_calc->encode(data);
        auto res =
            co_await run_for_all<address, std::shared_ptr<storage_interface>>(
                m_ioc,
                [&ctx, &encoded](size_t i, auto n) -> coro<address> {
                    co_return co_await n->write(ctx, encoded.get().at(i));
                },
                m_getter.get_services());

        address addr;
        for (const auto& a : res) {
            addr.append(a);
        }

        co_return addr;
    }

    coro<void> read_fragment(context& ctx, char* buffer,
                             const fragment& f) override {
        auto cl = m_getter.get(f.pointer);
        co_await cl->read_fragment(ctx, buffer, f);
    }

    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size) override {
        co_return co_await m_getter.get(pointer)->read(ctx, pointer, size);
    }

    coro<void> read_address(context& ctx, char* buffer, const address& addr,
                            const std::vector<size_t>& offsets) override {

        auto info = extract_node_address_map(addr, m_getter, offsets);

        std::vector<std::shared_ptr<awaitable_promise<void>>> promises;
        promises.reserve(info.nodes.size());

        for (auto& dn : info.nodes) {
            promises.emplace_back(
                std::make_shared<awaitable_promise<void>>(m_ioc));

            boost::asio::co_spawn(
                m_ioc,
                dn->read_address(ctx, buffer, info.node_address_map[dn],
                                 info.node_data_offsets_map[dn]),
                use_awaitable_promise_cospawn(promises.back()));
        }

        for (auto& p : promises) {
            co_await p->get();
        }
    }

    coro<address> link(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        std::map<size_t, address> addresses;
        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx, &addresses](auto id, auto dn,
                               const auto& addr) -> coro<void> {
                addresses.emplace(id, co_await dn->link(ctx, addr));
            });

        address rv;
        for (const auto& a : addresses) {
            rv.append(a.second);
        }

        co_return rv;
    }

    coro<void> unlink(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx](auto, auto dn, const auto& addr) -> coro<void> {
                co_await dn->unlink(ctx, addr);
            });
    }

    coro<void> sync(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx](auto, auto dn, const auto& addr) -> coro<void> {
                co_await dn->sync(ctx, addr);
            });
    }

    coro<size_t> get_used_space(context& ctx) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        auto nodes = m_getter.get_services();

        size_t used = 0;
        for (const auto& dn : nodes) {
            used += co_await dn->get_used_space(ctx);
        }
        co_return used;
    }

    void recover(const address& addr) {}

private:
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    storage_service_get_handler m_getter;
    std::unique_ptr<ec_interface> m_ec_calc;
    boost::asio::io_context& m_ioc;
    std::atomic<ec_status> m_status = empty;

    void update_status() {

        size_t count = 0;
        for (const auto& n : m_nodes) {
            if (n == nullptr) {
                count++;
            }
        }

        if (count == 0) {
            m_status = healthy;
        } else if (count == m_nodes.size()) {
            m_status = empty;
        } else {
            m_status = degraded;
        }
    }
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
