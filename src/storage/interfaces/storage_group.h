
#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H

#include "common/coroutines/coro_util.h"
#include "common/ec/ec_calculator.h"
#include "common/etcd/service_discovery/service_get_handler.h"

namespace uh::cluster {

struct storage_system_config {
    size_t data_nodes;
    size_t ec_nodes;
};

enum ec_status {
    degraded,
    healthy,
    recovering,
    empty,
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
          m_ec_calc(data_nodes, ec_nodes),
          m_ioc(ioc) {}

    coro<address> write(context& ctx, const std::string_view& data) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }
        auto encoded = m_ec_calc.encode(data);
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
        co_await m_getter.get(addr.get(0).pointer)
            ->read_address(ctx, buffer, addr, offsets);
    }

    coro<address> link(context& ctx, const address& addr) override {
        co_return co_await m_getter.get(addr.get(0).pointer)->link(ctx, addr);
    }

    coro<void> unlink(context& ctx, const address& addr) override {
        co_return co_await m_getter.get(addr.get(0).pointer)->unlink(ctx, addr);
    }

    coro<void> sync(context& ctx, const address& addr) override {
        co_return co_await m_getter.get(addr.get(0).pointer)->sync(ctx, addr);
    }

    coro<size_t> get_used_space(context& ctx) override {
        co_return co_await m_getter.get_services().back()->get_used_space(ctx);
    }

private:
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    service_get_handler<storage_interface> m_getter;
    ec_calculator m_ec_calc;
    boost::asio::io_context& m_ioc;
    ec_status m_status = empty;

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
