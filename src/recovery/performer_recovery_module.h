#ifndef PERFORMER_RECOVERY_MODULE_H
#define PERFORMER_RECOVERY_MODULE_H
#include "recovery_module.h"
#include "third-party/etcd-cpp-apiv3/etcd/SyncClient.hpp"

namespace uh::cluster {

struct recovery_module_factory;

class performer_recovery_module : public recovery_module {
public:
    void async_check_recover(std::atomic<ec_status>& status) override {
        boost::asio::co_spawn(
            m_ioc, recover(status), [](const std::exception_ptr& e) {
                if (e)
                    try {
                        std::rethrow_exception(e);
                    } catch (const std::exception& ex) {
                        LOG_ERROR() << "failure in recovery: " << ex.what();
                    }
            });
    }

private:
    struct recovery_info {
        std::vector<data_stat> stats{};
        std::size_t recover_size{};
        bool healthy = false;
        context ctx{};
    };

    coro<void> recover(std::atomic<ec_status>& status) {
        auto rinfo = co_await check_recovery();
        if (rinfo.healthy) {
            status = healthy;
            // TODO publish on etcd
            co_return;
        }

        auto nodes = m_getter.get_services();
        status = recovering;
        const auto ds_id_used_map = co_await get_ds_id_used_map(rinfo.ctx);
        unique_buffer<char> buf(RECOVERY_CHUNK_SIZE);
        uint128_t recovered_size = 0;
        while (recovered_size < rinfo.recover_size) {
            auto size = std::min(uint128_t{RECOVERY_CHUNK_SIZE},
                                 rinfo.recover_size - recovered_size)
                            .get_low();
            auto addr = calculate_recovery_address(recovered_size, size,
                                                   ds_id_used_map);
            size = addr.sizes.front();

            std::vector<size_t> offsets;
            std::vector<std::string_view> shards;
            offsets.reserve(m_getter.size());
            shards.reserve(nodes.size());
            size_t pointer = 0;
            for (size_t i = 0; i < nodes.size(); i++) {
                offsets.emplace_back(pointer);
                shards.emplace_back(buf.string_view().substr(pointer, size));
                pointer += size;
            }

            co_await perform_for_address(
                addr, m_getter, m_ioc,
                [&ctx = rinfo.ctx, &buf](auto, auto dn,
                                         const auto& info) -> coro<void> {
                    co_await dn->read_address(ctx, buf.data(), info.addr,
                                              info.pointer_offsets);
                },
                offsets);

            m_ec_calc.recover(shards, rinfo.stats);
            for (size_t i = 0; i < rinfo.stats.size(); i++) {
                if (rinfo.stats[i] == lost) {
                    co_await nodes.at(i)->write(rinfo.ctx, shards.at(i));
                }
            }
            recovered_size += size;
        }
        // TODO publish on etcd
        status = healthy;
    }

    coro<recovery_info> check_recovery() {

        std::map<size_t, std::vector<size_t>> sizes;
        context ctx;
        size_t i = 0;
        for (const auto& dn : m_getter.get_services()) {
            const auto size = co_await dn->get_used_space(ctx);
            sizes[size].emplace_back(i++);
        }

        if (sizes.size() > 2 or
            (sizes.size() == 2 and sizes.cbegin()->first != 0)) {
            throw std::logic_error("Undefined state");
        }

        recovery_info rinfo{.stats = {m_getter.size(), valid},
                            .recover_size = sizes.crbegin()->first,
                            .healthy = true,
                            .ctx = ctx};
        if (sizes.size() == 2) {
            for (const auto& fail_index : sizes[0]) {
                rinfo.stats[fail_index] = lost;
            }
            rinfo.healthy = false;
        }

        co_return rinfo;
    }

    address
    calculate_recovery_address(const uint128_t& recovered_size, size_t size,
                               const std::map<size_t, size_t>& ds_id_used_map) {
        address addr;

        auto ds = ds_id_used_map.cbegin();
        size_t internal_offset = pointer_traits::get_pointer(recovered_size);
        uint128_t upperbound_pointer = ds->second;

        while (upperbound_pointer <= recovered_size) {
            ++ds;
            internal_offset = pointer_traits::get_pointer(recovered_size -
                                                          upperbound_pointer);
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

    performer_recovery_module(storage_service_get_handler& getter,
                              boost::asio::io_context& ioc, ec_interface& ec,
                              etcd::SyncClient& etcd_client)
        : m_getter(getter),
          m_ioc(ioc),
          m_ec_calc(ec),
          m_etcd_client(etcd_client) {
        (void)m_etcd_client; // silence clang, please remove when m_etcd_client
                             // is used
    }

    storage_service_get_handler& m_getter;
    boost::asio::io_context& m_ioc;
    ec_interface& m_ec_calc;
    etcd::SyncClient& m_etcd_client;
    friend recovery_module_factory;
};

} // end namespace uh::cluster

#endif // PERFORMER_RECOVERY_MODULE_H
