#pragma once

#include "common/telemetry/log.h"
#include "common/utils/pointer_traits.h"
#include "common/utils/time_utils.h"

#include <storage/default_data_store.h>
#include <storage/interface.h>

#include <list>
#include <span>

namespace uh::cluster {

struct local_storage : public sn::interface {

    local_storage(uint32_t index, const data_store_config& config,
                  const std::list<std::filesystem::path>& paths)
        : m_threads(16 * paths.size()) {

        m_data_stores.reserve(paths.size());
        size_t i = 0;
        for (const auto& path : paths) {
            m_data_stores.emplace_back(
                std::make_unique<default_data_store>(config, path, index, i++));
        }
    }

    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override {

        const size_t part_size =
            std::ceil(static_cast<double>(data.size()) /
                      static_cast<double>(m_data_stores.size()));

        address total_addr;
        std::size_t offsets_idx = 0;
        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            const std::size_t start_pos = i * part_size;
            const std::size_t current_part_size =
                std::min(data.size() - start_pos, part_size);

            std::vector<std::size_t> local_offsets;
            while (offsets_idx < offsets.size() &&
                   offsets[offsets_idx] < start_pos + current_part_size) {
                local_offsets.push_back(offsets[offsets_idx] - start_pos);
                ++offsets_idx;
            }

            auto addr = m_data_stores[i]->write(
                data.subspan(start_pos, current_part_size), local_offsets);
            total_addr.append(addr);
        }

        co_return total_addr;
    }

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override {
        return read_address(ctx, addr, buffer);
    }

    coro<std::size_t> read_address(context& ctx, const address& addr,
                                   std::span<char> buffer) {
        LOG_DEBUG() << ctx.peer() << ": read addr start";

        std::size_t offs = 0ull;

        for (size_t i = 0; i < addr.size(); i++) {
            auto frag = addr.get(i);
            auto& ds = get_data_store(frag.pointer);

            auto count = ds.read(frag.pointer, buffer.subspan(offs, frag.size));
            offs += count;

            if (count != frag.size) {
                break;
            }
        }

        LOG_DEBUG() << ctx.peer() << ": read addr done";
        co_return offs;
    }

    coro<address> link(context& ctx, const address& addr) override {

        std::vector<address> ds_addresses(m_data_stores.size());
        for (size_t i = 0; i < addr.size(); i++) {
            const auto f = addr.get(i);
            const auto id = pointer_traits::get_data_store_id(f.pointer);
            ds_addresses.at(id).push(f);
        }

        std::vector<std::future<address>> futures;
        futures.reserve(m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {

            auto p = std::make_shared<std::promise<address>>();
            boost::asio::post(m_threads, [i, this, p, &ds_addresses]() {
                try {
                    p->set_value(m_data_stores[i]->link(ds_addresses[i]));
                } catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());
        }

        address rv;
        for (auto& f : futures) {
            rv.append(f.get());
        }

        co_return rv;
    }

    coro<std::size_t> unlink(context& ctx, const address& addr) override {

        std::vector<address> ds_addresses(m_data_stores.size());
        for (size_t i = 0; i < addr.size(); i++) {
            const auto f = addr.get(i);
            const auto id = pointer_traits::get_data_store_id(f.pointer);
            ds_addresses.at(id).push(f);
        }

        std::vector<std::future<std::size_t>> futures;
        futures.reserve(m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {

            auto p = std::make_shared<std::promise<std::size_t>>();
            boost::asio::post(m_threads, [i, this, p, &ds_addresses]() {
                try {
                    p->set_value(m_data_stores[i]->unlink(ds_addresses[i]));
                } catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());
        }

        std::size_t freed_bytes = 0;
        for (auto& f : futures) {
            freed_bytes += f.get();
        }

        co_return freed_bytes;
    }

    coro<size_t> get_used_space(context& ctx) override {

        size_t used = 0;
        for (const auto& ds : m_data_stores) {
            used += ds->get_used_space();
        }
        co_return used;
    }

    coro<std::map<size_t, size_t>> get_ds_size_map(context& ctx) override {
        std::map<size_t, size_t> res;
        for (const auto& ds : m_data_stores) {
            res.emplace(ds->id(), ds->get_used_space());
        }
        co_return res;
    }

    coro<void> ds_write(context& ctx, uint32_t ds_id, uint64_t pointer,
                        std::span<const char> data) override {
        m_data_stores.at(ds_id)->manual_write(pointer, data);
        co_return;
    }

    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       size_t size, char* buffer) override {
        m_data_stores.at(ds_id)->manual_read(pointer, {buffer, size});
        co_return;
    }

    size_t get_free_space() {

        size_t free = 0;
        for (const auto& ds : m_data_stores) {
            free += ds->get_available_space();
        }
        return free;
    }

private:
    std::vector<std::unique_ptr<data_store>> m_data_stores;
    boost::asio::thread_pool m_threads;

    data_store& get_data_store(const uint128_t& pointer) const {
        auto data_store_id = pointer_traits::get_data_store_id(pointer);

        if (data_store_id >= m_data_stores.size()) {
            throw std::runtime_error("data store id out of range");
        }

        return *m_data_stores[data_store_id];
    }
};

} // namespace uh::cluster
