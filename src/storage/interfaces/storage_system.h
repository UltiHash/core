//
// Created by massi on 7/31/24.
//

#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H

#include "common/coroutines/coro_util.h"
#include "common/ec/ec_group.h"
#include "common/etcd/service_discovery/service_basic_getter.h"

namespace uh::cluster {

class ec_calculator {
    const size_t m_data_nodes;
    const size_t m_ec_nodes;

    struct encoded {
        [[nodiscard]] std::vector<std::string_view> get() const noexcept {
            return m_encoded;
        }

    private:
        friend ec_calculator;
        encoded() = default;
        std::list<unique_buffer<>> m_parities{};
        std::vector<std::string_view> m_encoded;
    };

public:
    ec_calculator(size_t data_nodes, size_t ec_nodes)
        : m_data_nodes(data_nodes),
          m_ec_nodes(ec_nodes) {}

    [[nodiscard]] encoded encode(const std::string_view&) const { return {}; }
};

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

struct storage_system : public storage_interface {

private:
    ec_status m_status = empty;
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    service_basic_getter<storage_interface> m_getter;

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

public:
    void insert(size_t i, const std::shared_ptr<storage_interface>& node) {
        m_nodes.at(i) = node;
        m_getter.add_client(i, node);
        update_status();
    }

    void remove(size_t i) {
        m_getter.remove_client(i, m_nodes.at(i));
        m_nodes.at(i) = nullptr;
        m_status = degraded;
    }

    [[nodiscard]] bool is_healthy() const noexcept {
        return m_status == healthy;
    }

    [[nodiscard]] bool is_empty() const noexcept { return m_status == empty; }
    storage_system(boost::asio::io_context& ioc, size_t data_nodes,
                   size_t ec_nodes)
        : m_nodes(data_nodes + ec_nodes),
          m_ec_calc(data_nodes, ec_nodes),
          m_ioc(ioc) {}

    coro<address> write(context& ctx, const std::string_view& data) override {

        if (is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }
        auto encoded = m_ec_calc.encode(data);
        auto res =
            co_await run_for_all<address, std::shared_ptr<storage_interface>>(
                m_ioc,
                [&ctx, &encoded](size_t i, auto n) {
                    return n->write(ctx, encoded.get().at(i));
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
        co_return;
    }
    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size) override {
        co_return shared_buffer{};
    }
    coro<void> read_address(context& ctx, char* buffer, const address& addr,
                            const std::vector<size_t>& offsets) override {
        co_return;
    }

    coro<void> link(context& ctx, const address& addr) override { co_return; }
    coro<void> unlink(context& ctx, const address& addr) override { co_return; }
    coro<void> sync(context& ctx, const address& addr) override { co_return; }
    coro<size_t> get_used_space(context& ctx) override { co_return 0; }

private:
    ec_calculator m_ec_calc;
    boost::asio::io_context& m_ioc;
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
