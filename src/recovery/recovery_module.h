#ifndef PERFORMER_RECOVERY_MODULE_H
#define PERFORMER_RECOVERY_MODULE_H
#include "common/coroutines/coro_util.h"
#include "common/ec/ec_interface.h"
#include "common/etcd/ec_groups/ec_group_attributes.h"
#include "common/etcd/service_discovery/storage_service_get_handler.h"
#include "common/telemetry/log.h"
#include "common/utils/address_utils.h"
#include "common/utils/pointer_traits.h"

#include <boost/asio/co_spawn.hpp>

namespace uh::cluster {

class recovery_module {
public:
    recovery_module(storage_service_get_handler& getter,
                    boost::asio::io_context& ioc, ec_interface& ec,
                    ec_group_attributes& attributes)
        : m_getter(getter),
          m_ioc(ioc),
          m_ec_calc(ec),
          m_attributes(attributes) {}

    ~recovery_module() { m_attributes.clear(); }

    void async_check_recover(std::atomic<ec_status>& status,
                             size_t group_size) {
        if (m_getter.size() == 0) {
            set_status(status, empty);
        } else if (m_getter.size() < group_size) {
            set_status(status, degraded);
        } else {
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
    }

private:
    struct recovery_info {
        std::vector<data_stat> stats{};
        std::size_t recover_size{};
        bool healthy = false;
        context ctx{};
    };

    coro<void> recover(std::atomic<ec_status>& status) {
        LOG_INFO() << "start check recovery";
        auto rinfo = co_await check_recovery();
        if (rinfo.healthy) {
            LOG_INFO() << "status healthy for group "
                       << m_attributes.group_id();
            set_status(status, healthy);
            co_return;
        }

        LOG_INFO() << "starting recovery for group " << m_attributes.group_id();

        set_status(status, recovering);

        auto nodes = m_getter.get_services();
        const auto ds_id_used_map = co_await get_ds_id_used_map(rinfo);
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
        set_status(status, healthy);
    }

    void set_status(std::atomic<ec_status>& status, ec_status value) {
        m_attributes.set_status(value);
        status = value;
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

    coro<std::map<size_t, size_t>> get_ds_id_used_map(recovery_info& rinfo) {
        const auto maps = co_await run_for_all<
            std::map<size_t, size_t>, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&ctx = rinfo.ctx](size_t, std::shared_ptr<storage_interface> dn)
                -> coro<std::map<size_t, size_t>> {
                if (!dn) {
                    throw std::runtime_error("inaccessible data service");
                }
                co_return co_await dn->get_ds_size_map(ctx);
            },
            m_getter.get_services());

        auto f = std::ranges::find(rinfo.stats, valid);
        if (f == rinfo.stats.cend()) {
            throw std::logic_error("no valid nodes are found");
        }

        const size_t index = std::distance(f, rinfo.stats.begin());
        const auto& map1 = maps.at(index);
        for (size_t i = index + 1; i < maps.size(); i++) {
            const auto& map2 = maps.at(i);
            if (rinfo.stats.at(i) == valid and
                !std::ranges::equal(map1, map2)) {
                throw std::runtime_error(
                    "inconsistent state of used spaces in different data "
                    "stores in storage services");
            }
        }

        co_return maps.front();
    }

    storage_service_get_handler& m_getter;
    boost::asio::io_context& m_ioc;
    ec_interface& m_ec_calc;
    ec_group_attributes& m_attributes;
};

} // end namespace uh::cluster

#endif // PERFORMER_RECOVERY_MODULE_H
