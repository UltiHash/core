
#ifndef STORAGE_SYSTEM_H
#define STORAGE_SYSTEM_H

#include "common/coroutines/coro_util.h"
#include "common/ec/ec_factory.h"
#include "common/ec/reedsolomon_c.h"
#include "common/etcd/service_discovery/storage_service_get_handler.h"
#include "common/utils/address_utils.h"

namespace uh::cluster {

enum ec_status {
    empty,
    degraded,
    complete,
    recovering,
    healthy,
    failed_recovery,
};

struct storage_group : public storage_interface {

    storage_group(boost::asio::io_context& ioc, size_t data_nodes,
                  size_t ec_nodes)
        : m_nodes(data_nodes + ec_nodes),
          m_ec_calc(ec_factory::create(data_nodes, ec_nodes)),
          m_ioc(ioc) {}

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

    coro<void> unlink(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx](auto, auto dn, const auto& info) -> coro<void> {
                co_await dn->unlink(ctx, info.addr);
            });
    }

    coro<void> sync(context& ctx, const address& addr) override {

        if (!is_healthy()) {
            throw std::runtime_error("unhealthy storage system");
        }

        co_await perform_for_address(
            addr, m_getter, m_ioc,
            [&ctx](auto, auto dn, const auto& info) -> coro<void> {
                co_await dn->sync(ctx, info.addr);
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

    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override {
        throw std::runtime_error(
            "This operation is not allowed in storage group");
    }

private:
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    storage_service_get_handler m_getter;
    std::unique_ptr<ec_interface> m_ec_calc;
    boost::asio::io_context& m_ioc;
    std::atomic<ec_status> m_status = empty;

    struct recovery_info {
        std::vector<data_stat> stats;
        std::size_t recover_size;
        bool healthy = false;
        context ctx;
    };

    address
    calculate_recovery_address(const uint128_t& offset, size_t size,
                               const std::map<size_t, size_t>& ds_id_used_map) {
        address addr;

        auto ds = ds_id_used_map.cbegin();
        uint128_t upperbound_pointer = ds->second;
        size_t internal_offset = offset.get_low();
        while (upperbound_pointer <= offset) {
            ++ds;
            internal_offset = (offset - upperbound_pointer).get_low();
            upperbound_pointer += ds->second;
        }
        size_t ds_bounded_size = std::min(size, ds->second - internal_offset);

        for (const auto id : m_getter.get_storage_ids()) {
            const auto pointer = pointer_traits::get_global_pointer(
                internal_offset, id, ds->first);
            addr.push({pointer, ds_bounded_size});
        }
        return addr;
    }

    coro<std::map<size_t, size_t>> get_ds_id_used_map(context& ctx) {
        const auto maps =
            co_await run_for_all<std::map<size_t, size_t>,
                                 std::shared_ptr<storage_interface>>(
                m_ioc,
                [&ctx](size_t, std::shared_ptr<storage_interface> dn)
                    -> coro<std::map<size_t, size_t>> {
                    if (!dn) {
                        throw std::runtime_error("inaccessible data service");
                    }
                    co_return co_await dn->get_ds_size_map(ctx);
                },
                m_getter.get_services());

        const auto& map1 = maps.front();
        for (size_t i = 1; i < maps.size(); i++) {
            const auto& map2 = maps.at(i);
            if (!std::ranges::equal(map1, map2)) {
                throw std::runtime_error(
                    "inconsistent state of used spaces in different data "
                    "stores in storage services");
            }
        }

        co_return maps.front();
    }

    coro<void> recover(recovery_info& rinfo) {
        m_status = recovering;
        const auto ds_id_used_map = co_await get_ds_id_used_map(rinfo.ctx);
        unique_buffer buf(RECOVERY_CHUNK_SIZE);
        uint128_t offset = 0;
        while (offset < rinfo.recover_size) {
            auto size = std::min(uint128_t{RECOVERY_CHUNK_SIZE},
                                 rinfo.recover_size - offset)
                            .get_low();
            auto addr =
                calculate_recovery_address(offset, size, ds_id_used_map);
            size = addr.sizes.front();

            std::vector<size_t> offsets;
            std::vector<std::string_view> shards;
            offsets.reserve(m_nodes.size());
            shards.reserve(m_nodes.size());
            size_t pointer = 0;
            for (size_t i = 0; i < m_nodes.size(); i++) {
                offsets.emplace_back(pointer);
                shards.emplace_back(buf.string_view().substr(pointer, size));
                pointer += size;
            }

            co_await read_address(rinfo.ctx, buf.data(), addr, offsets);
            m_ec_calc->recover(shards, rinfo.stats);
            for (size_t i = 0; i < rinfo.stats.size(); i++) {
                if (rinfo.stats[i] == lost) {
                    co_await m_nodes.at(i)->write(rinfo.ctx, shards.at(i));
                }
            }
            offset += size;
        }

        m_status = healthy;
    }

    void update_status() {

        size_t count = 0;
        for (const auto& n : m_nodes) {
            if (n == nullptr) {
                count++;
            }
        }

        if (count == 0) {
            auto rinfo = check_recovery();

            if (rinfo.healthy) {
                m_status = healthy;
            } else {
                m_status = complete;
                if (global_service_role == RECOVERY_SERVICE) {
                    boost::asio::co_spawn(
                        m_ioc, recover(rinfo), [](const std::exception_ptr& e) {
                            if (e)
                                try {
                                    std::rethrow_exception(e);
                                } catch (const std::exception& ex) {
                                    LOG_ERROR()
                                        << "failure in recovery: " << ex.what();
                                }
                        });
                }
            }
        } else if (count == m_nodes.size()) {
            m_status = empty;
        } else {
            m_status = degraded;
        }
    }

    recovery_info check_recovery() {
        auto nodes = m_getter.get_services();

        std::map<size_t, std::vector<size_t>> sizes;
        context ctx;
        size_t i = 0;
        for (const auto& dn : nodes) {
            const auto size =
                boost::asio::co_spawn(m_ioc, dn->get_used_space(ctx),
                                      boost::asio::use_future)
                    .get();
            sizes[size].emplace_back(i++);
        }

        if (sizes.size() > 2 or
            (sizes.size() == 2 and sizes.cbegin()->first != 0)) {
            throw std::logic_error("Undefined state");
        }

        recovery_info rinfo{.stats = {nodes.size(), valid},
                            .recover_size = sizes.crbegin()->first,
                            .healthy = true,
                            .ctx = ctx};
        if (sizes.size() == 2) {
            for (const auto& fail_index : sizes[0]) {
                rinfo.stats[fail_index] = lost;
            }
            rinfo.healthy = false;
        }

        return rinfo;
    }
};

} // namespace uh::cluster

#endif // STORAGE_SYSTEM_H
