#pragma once

#include "common/service_interfaces/storage_interface.h"
#include "common/telemetry/log.h"
#include "common/utils/pointer_traits.h"
#include "common/utils/time_utils.h"
#include "storage/default_data_store.h"
#include <list>
#include <string_view>

namespace uh::cluster {

struct load_monitor {
    explicit load_monitor(std::atomic<double>& load)
        : m_load(load) {}
    ~load_monitor() { m_load += m_timer.passed().count(); }
    std::atomic<double>& m_load;
    timer m_timer;
};

struct local_storage : public storage_interface {

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

    coro<address> write(context& ctx, const std::string_view& data,
                        const std::vector<std::size_t>& offsets) override {

        load_monitor load(m_load);
        const size_t part_size =
            std::ceil(static_cast<double>(data.size()) /
                      static_cast<double>(m_data_stores.size()));

        address total_addr;
        std::size_t offsets_idx = 0;
        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            const std::size_t start_pos = i * part_size;
            const std::size_t current_part_size =
                std::min(data.size() - start_pos, part_size);
            const std::string_view part =
                data.substr(start_pos, current_part_size);

            std::vector<std::size_t> local_offsets;
            while (offsets_idx < offsets.size() &&
                   offsets[offsets_idx] < start_pos + current_part_size) {
                local_offsets.push_back(offsets[offsets_idx] - start_pos);
                ++offsets_idx;
            }
            auto addr = m_data_stores[i]->write(part, local_offsets);
            total_addr.append(addr);
        }

        co_return total_addr;
    }

    coro<void> read_fragment(context& ctx, char* buffer,
                             const fragment& f) override {
        load_monitor load(m_load);
        get_data_store(f.pointer).read(buffer, f.pointer, f.size);
        co_return;
    }

    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size) override {
        load_monitor load(m_load);
        shared_buffer<> buf(size);
        const auto read_size =
            get_data_store(pointer).read_up_to(buf.data(), pointer, size);
        buf.resize(read_size);
        co_return buf;
    }

    coro<void> read_address(context& ctx, char* buffer, const address& addr,
                            const std::vector<size_t>& offsets) override {
        load_monitor load(m_load);
        LOG_DEBUG() << ctx.peer() << ": read addr start";
        for (size_t i = 0; i < addr.size(); i++) {
            const auto frag = addr.get(i);
            if (get_data_store(frag.pointer)
                    .read(buffer + offsets[i], frag.pointer, frag.size) !=
                frag.size) [[unlikely]] {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
        }
        LOG_DEBUG() << ctx.peer() << ": read addr done";
        co_return;
    }

    coro<address> link(context& ctx, const address& addr) override {
        load_monitor load(m_load);

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
        load_monitor load(m_load);

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
        load_monitor load(m_load);

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
                        const std::string_view& data) override {
        m_data_stores.at(ds_id)->manual_write(pointer, data);
        co_return;
    }

    coro<void> ds_read(context& ctx, uint32_t ds_id, uint64_t pointer,
                       size_t size, char* buffer) override {
        m_data_stores.at(ds_id)->manual_read(pointer, size, buffer);
        co_return;
    }

    size_t get_free_space() {
        load_monitor load(m_load);

        size_t free = 0;
        for (const auto& ds : m_data_stores) {
            free += ds->get_available_space();
        }
        return free;
    }

    double catch_load() { return m_load.exchange(0); }

private:
    std::vector<std::unique_ptr<default_data_store>> m_data_stores;
    boost::asio::thread_pool m_threads;
    std::atomic<double> m_load;

    [[nodiscard]] default_data_store&
    get_data_store(const uint128_t& pointer) const {
        auto data_store_id = pointer_traits::get_data_store_id(pointer);

        if (data_store_id >= m_data_stores.size()) {
            throw std::runtime_error("data store id out of range");
        }

        return *m_data_stores[data_store_id];
    }
};

} // namespace uh::cluster
