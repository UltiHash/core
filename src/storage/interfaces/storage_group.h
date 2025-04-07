#pragma once

#include <common/coroutines/coro_util.h>
#include <common/ec/ec_factory.h>
#include <common/ec/reedsolomon_c.h>
#include <common/etcd/service_discovery/storage_service_get_handler.h>
#include <coordinator/recovery_module.h>
#include <storage/address_utils.h>

namespace uh::cluster {

struct storage_group : public storage_interface {

    storage_group(boost::asio::io_context& ioc, size_t data_nodes,
                  size_t ec_nodes, size_t group_id, etcd_manager& etcd,
                  bool active_recovery);

    void insert(size_t id, size_t group_nid,
                const std::shared_ptr<storage_interface>& node);

    void remove(size_t id, size_t group_nid);

    [[nodiscard]] bool is_healthy() const noexcept;

    [[nodiscard]] bool is_empty() const noexcept;

    coro<address> write(std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override;

    coro<void> read_fragment(char* buffer, const fragment& f) override;

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size) override;

    coro<void> read_address(const address& addr, std::span<char> buffer,
                            const std::vector<size_t>& offsets) override;

    coro<address> link(const address& addr) override;

    coro<std::size_t> unlink(const address& addr) override;

    coro<size_t> get_used_space() override;

    coro<std::map<size_t, size_t>> get_ds_size_map() override;

    [[nodiscard]] size_t group_id() const noexcept;

    coro<void> ds_write(uint32_t ds_id, uint64_t pointer,
                        std::span<const char>) override;

    coro<void> ds_read(uint32_t ds_id, uint64_t pointer, size_t size,
                       char* buffer) override;

private:
    std::vector<std::shared_ptr<storage_interface>> m_nodes;
    storage_service_get_handler m_getter;
    std::unique_ptr<ec_interface> m_ec_calc;
    boost::asio::io_context& m_ioc;
    std::atomic<ec_status> m_status = empty;
    ec_group_attributes m_attributes;
    etcd_manager::watch_guard m_watch_guard;
    std::optional<recovery_module> m_rec_mod;
    std::mutex m_mutex;
};

} // namespace uh::cluster
