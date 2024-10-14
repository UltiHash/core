
#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H

#include "common/coroutines/coro_util.h"
#include "common/ec/ec_factory.h"
#include "common/ec/reedsolomon_c.h"
#include "common/etcd/ec_groups/status_watcher.h"
#include "common/etcd/service_discovery/storage_service_get_handler.h"
#include "common/utils/address_utils.h"
#include "recovery/recovery_module.h"

namespace uh::cluster {

struct storage_group : public storage_interface {

    storage_group(boost::asio::io_context& ioc, size_t data_nodes,
                  size_t ec_nodes, size_t group_id, etcd::SyncClient& etcd,
                  bool active_recovery)
        : m_nodes(data_nodes + ec_nodes),
          m_ec_calc(ec_factory::create(data_nodes, ec_nodes)),
          m_ioc(ioc),
          m_attributes(group_id, etcd) {
        if (!active_recovery) {
            if (data_nodes == 1 and ec_nodes == 0) {
                m_status = healthy;
            }
            else {
                m_status_watcher.emplace(m_attributes, m_status);
            }
        } else {
            m_rec_mod.emplace(m_getter, m_ioc, *m_ec_calc, m_attributes);
        }
    }

    void insert(size_t id, size_t group_nid,
                const std::shared_ptr<storage_interface>& node) {
        m_nodes.at(group_nid) = node;
        m_getter.add_client(id, node);
        if (m_rec_mod) {
            m_rec_mod->async_check_recover(m_status, m_nodes.size());
        }
    }

    void remove(size_t id, size_t group_nid) {
        m_getter.remove_client(id, m_nodes.at(group_nid));
        m_nodes.at(group_nid) = nullptr;
        if (m_rec_mod) {
            m_rec_mod->async_check_recover(m_status, m_nodes.size());
        }
    }

    [[nodiscard]] bool is_healthy() const noexcept {
        return m_status == healthy;
    }

    [[nodiscard]] bool is_empty() const noexcept { return m_status == empty; }

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

        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx, &buffer](auto, auto dn, const auto& info) -> coro<void> {
                co_await dn->read_address(ctx, buffer, info.addr,
                                          info.pointer_offsets);
            },
            offsets);
    }

    coro<address> link(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        std::map<size_t, address> addresses;
        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx, &addresses](auto id, auto dn,
                               const auto& info) -> coro<void> {
                addresses.emplace(id, co_await dn->link(ctx, info.addr));
            });

        address rv;
        for (const auto& a : addresses) {
            rv.append(a.second);
        }

        co_return rv;
    }

    coro<std::size_t> unlink(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        std::atomic<size_t> freed_bytes;
        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx, &freed_bytes](auto, auto dn, const auto& info) -> coro<void> {
                freed_bytes += co_await dn->unlink(ctx, info.addr);
            });
        co_return freed_bytes;
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

    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override {
        throw std::runtime_error(
            "This operation is not allowed in storage group");
    }

    [[nodiscard]] size_t group_id() const noexcept {
        return m_attributes.group_id();
    }

    coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                        const std::string_view&) override {
        throw std::runtime_error("unsupported operation in storage group");
    }

    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       size_t size, char* buffer) override {
        throw std::runtime_error("unsupported operation in storage group");
    }


private:
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    storage_service_get_handler m_getter;
    std::unique_ptr<ec_interface> m_ec_calc;
    boost::asio::io_context& m_ioc;
    std::atomic<ec_status> m_status = empty;
    ec_group_attributes m_attributes;
    std::optional<status_watcher> m_status_watcher;
    std::optional<recovery_module> m_rec_mod;
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
