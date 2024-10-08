
#ifndef UH_CLUSTER_LOCAL_STORAGE_H
#define UH_CLUSTER_LOCAL_STORAGE_H

#include "common/utils/pointer_traits.h"
#include "common/utils/time_utils.h"
#include "storage/data_store.h"
#include <string_view>
#include <utility>

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
                std::make_unique<data_store>(config, path, index, i++));
        }
    }

    coro<address> write(context& ctx, const std::string_view& data) override {

        load_monitor load(m_load);
        const size_t part =
            std::ceil(static_cast<double>(data.size()) /
                      static_cast<double>(m_data_stores.size()));

        address total_addr;
        for (size_t i = 0; i < m_data_stores.size(); ++i) {
            const auto part_size = std::min(data.size() - i * part, part);
            auto addr = m_data_stores[i]->register_write(
                data.substr(i * part, part_size));
            total_addr.append(addr);
            boost::asio::post(m_threads, [addr = std::move(addr), i, this]() {
                m_data_stores[i]->perform_write(addr);
            });
        }
        co_return total_addr;
    }

    coro<void> read_fragment(context& ctx, char* buffer,
                             const fragment& f) override {
        load_monitor load(m_load);
        LOG_DEBUG() << ctx.peer() << ": read fragment start(" << f.to_string()
                    << ")";
        get_data_store(f.pointer).read(buffer, f.pointer, f.size);
        LOG_DEBUG() << ctx.peer() << ": read fragment done";
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
        for (size_t i = 0; i < addr.size(); i++) {
            const auto frag = addr.get(i);
            if (get_data_store(frag.pointer)
                    .read(buffer + offsets[i], frag.pointer, frag.size) !=
                frag.size) [[unlikely]] {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
        }
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

    coro<void> unlink(context& ctx, const address& addr) override {
        load_monitor load(m_load);

        std::vector<address> ds_addresses(m_data_stores.size());
        for (size_t i = 0; i < addr.size(); i++) {
            const auto f = addr.get(i);
            const auto id = pointer_traits::get_data_store_id(f.pointer);
            ds_addresses.at(id).push(f);
        }

        std::vector<std::future<void>> futures;
        futures.reserve(m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {

            auto p = std::make_shared<std::promise<void>>();
            boost::asio::post(m_threads, [i, this, p, &ds_addresses]() {
                try {
                    m_data_stores[i]->unlink(ds_addresses[i]);
                    p->set_value();
                } catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());
        }
        for (auto& f : futures) {
            f.get();
        }

        co_return;
    }

    coro<void> sync(context& ctx, const address& addr) override {
        load_monitor load(m_load);

        std::vector<address> ds_addresses(m_data_stores.size());
        for (size_t i = 0; i < addr.size(); i++) {
            const auto f = addr.get(i);
            const auto id = pointer_traits::get_data_store_id(f.pointer);
            ds_addresses.at(id).push(f);
        }

        std::vector<std::future<void>> futures;
        futures.reserve(m_data_stores.size());
        for (size_t i = 0; i < m_data_stores.size(); ++i) {

            auto p = std::make_shared<std::promise<void>>();
            boost::asio::post(m_threads, [i, this, p, &ds_addresses]() {
                try {
                    m_data_stores[i]->wait_for_ongoing_writes(ds_addresses[i]);
                    m_data_stores[i]->sync();
                    p->set_value();
                } catch (const std::exception&) {
                    p->set_exception(std::current_exception());
                }
            });
            futures.emplace_back(p->get_future());
        }
        for (auto& f : futures) {
            f.get();
        }

        co_return;
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
    std::vector<std::unique_ptr<data_store>> m_data_stores;
    boost::asio::thread_pool m_threads;
    std::atomic<double> m_load;

    [[nodiscard]] data_store& get_data_store(const uint128_t& pointer) const {
        return *m_data_stores[pointer_traits::get_data_store_id(pointer)];
    }
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LOCAL_STORAGE_H
