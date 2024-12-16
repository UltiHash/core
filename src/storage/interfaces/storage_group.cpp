#include "storage_group.h"

namespace uh::cluster {

storage_group::storage_group(boost::asio::io_context& ioc, size_t data_nodes,
                             size_t ec_nodes, size_t group_id,
                             etcd::SyncClient& etcd, bool active_recovery)
    : m_nodes(data_nodes + ec_nodes),
      m_ec_calc(ec_factory::create(data_nodes, ec_nodes)),
      m_ioc(ioc),
      m_attributes(group_id, etcd) {
    if (!active_recovery) {
        if (data_nodes == 1 and ec_nodes == 0) {
            m_status = healthy;
        } else {
            m_status_watcher.emplace(m_attributes, m_status);
        }
    } else {
        m_rec_mod.emplace(m_getter, m_ioc, *m_ec_calc, m_attributes);
    }
}

void storage_group::insert(size_t id, size_t group_nid,
                           const std::shared_ptr<storage_interface>& node) {
    m_nodes.at(group_nid) = node;
    m_getter.add_client(id, node);
    if (m_rec_mod) {
        m_rec_mod->async_check_recover(m_status, m_nodes.size());
    }
}

void storage_group::remove(size_t id, size_t group_nid) {
    m_getter.remove_client(id, m_nodes.at(group_nid));
    m_nodes.at(group_nid) = nullptr;
    if (m_rec_mod) {
        m_rec_mod->async_check_recover(m_status, m_nodes.size());
    }
}

[[nodiscard]] bool storage_group::is_healthy() const noexcept {
    return m_status == healthy;
}

[[nodiscard]] bool storage_group::is_empty() const noexcept {
    return m_status == empty;
}

coro<address> storage_group::write(context& ctx, const std::string_view& data,
                                   const std::vector<std::size_t>& offsets) {

    if (!is_healthy()) {
        throw std::runtime_error("unhealthy storage system");
    }
    auto encoded = m_ec_calc->encode(data);
    auto res =
        co_await run_for_all<address, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&ctx, &encoded, &offsets](size_t i, auto n) -> coro<address> {
                // TODO offsets need to be computed to match encoded EC data
                co_return co_await n->write(ctx, encoded.get().at(i), offsets);
            },
            m_getter.get_services());

    address addr;
    for (const auto& a : res) {
        addr.append(a);
    }

    co_return addr;
}

coro<void> storage_group::read_fragment(context& ctx, char* buffer,
                                        const fragment& f) {
    auto cl = m_getter.get(f.pointer);
    co_await cl->read_fragment(ctx, buffer, f);
}

coro<shared_buffer<>>
storage_group::read(context& ctx, const uint128_t& pointer, size_t size) {
    co_return co_await m_getter.get(pointer)->read(ctx, pointer, size);
}

coro<void> storage_group::read_address(context& ctx, char* buffer,
                                       const address& addr,
                                       const std::vector<size_t>& offsets) {

    co_await perform_for_address(
        addr, m_getter, m_ioc,
        [&ctx, &buffer](auto, auto dn, const auto& info) -> coro<void> {
            co_await dn->read_address(ctx, buffer, info.addr,
                                      info.pointer_offsets);
        },
        offsets);
}

coro<address> storage_group::link(context& ctx, const address& addr) {

    if (!is_healthy()) {
        throw std::runtime_error("unhealthy storage system");
    }

    std::map<size_t, address> addresses;
    co_await perform_for_address(
        addr, m_getter, m_ioc,
        [&ctx, &addresses](auto id, auto dn, const auto& info) -> coro<void> {
            addresses.emplace(id, co_await dn->link(ctx, info.addr));
        });

    address rv;
    for (const auto& a : addresses) {
        rv.append(a.second);
    }

    co_return rv;
}

coro<std::size_t> storage_group::unlink(context& ctx, const address& addr) {

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

coro<size_t> storage_group::get_used_space(context& ctx) {

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

coro<std::map<size_t, size_t>> storage_group::get_ds_size_map(context& ctx) {
    throw std::runtime_error("This operation is not allowed in storage group");
}

[[nodiscard]] size_t storage_group::group_id() const noexcept {
    return m_attributes.group_id();
}

coro<void> storage_group::ds_write(context& ctx, uint32_t ds_id,
                                   uint64_t pointer, const std::string_view&) {
    throw std::runtime_error("unsupported operation in storage group");
}

coro<void> storage_group::ds_read(context& ctx, uint32_t ds_id,
                                  uint64_t pointer, size_t size, char* buffer) {
    throw std::runtime_error("unsupported operation in storage group");
}

} // namespace uh::cluster
