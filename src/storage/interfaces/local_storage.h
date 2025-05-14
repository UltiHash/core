#pragma once

#include "common/service_interfaces/storage_interface.h"
#include "common/telemetry/log.h"
#include "common/utils/pointer_traits.h"
#include "common/utils/time_utils.h"
#include "storage/default_data_store.h"
#include <list>
#include <span>

namespace uh::cluster {

struct local_storage : public storage_interface {

    local_storage(uint32_t index, const data_store_config& config,
                  const std::filesystem::path& path)
        : m_threads(16),
          m_data_store(
              std::make_unique<default_data_store>(config, path, index)) {}

    coro<address> write(allocation_t allocation, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override {
        co_return m_data_store->write(allocation, data, offsets);
    }

    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size) override {
        shared_buffer<> buf(size);
        auto read_size = m_data_store->read(
            pointer_traits::get_pointer(pointer), buf.span());
        buf.resize(read_size);
        co_return buf;
    }

    coro<void> read_address(const address& addr, std::span<char> buffer,
                            const std::vector<size_t>& offsets) override {
        LOG_DEBUG() << "read addr start";

        for (size_t i = 0; i < addr.size(); i++) {
            const auto frag = addr.get(i);
            if (m_data_store->read(pointer_traits::get_pointer(frag.pointer),
                                   buffer.subspan(offsets[i], frag.size)) !=
                frag.size) {
                throw std::runtime_error(
                    "Could not read the data with the given size");
            }
        }

        LOG_DEBUG() << "read addr done";
        co_return;
    }

    coro<address> link(const address& addr) override {
        auto p = std::make_shared<std::promise<address>>();
        boost::asio::post(m_threads, [this, p, &addr]() {
            try {
                p->set_value(m_data_store->link(addr));
            } catch (const std::exception&) {
                p->set_exception(std::current_exception());
            }
        });
        co_return p->get_future().get();
    }

    coro<std::size_t> unlink(const address& addr) override {
        auto p = std::make_shared<std::promise<std::size_t>>();
        boost::asio::post(m_threads, [this, p, &addr]() {
            try {
                p->set_value(m_data_store->unlink(addr));
            } catch (const std::exception&) {
                p->set_exception(std::current_exception());
            }
        });
        co_return p->get_future().get();
    }

    std::size_t get_used_space_func() { return m_data_store->get_used_space(); }

    coro<std::size_t> get_used_space() override {
        co_return get_used_space_func();
    }

    std::size_t get_available_space_func() {
        return m_data_store->get_available_space();
    }

    coro<allocation_t> allocate(std::size_t size) override {
        co_return m_data_store->allocate(size);
    }

private:
    boost::asio::thread_pool m_threads;
    std::unique_ptr<data_store> m_data_store;
};

} // namespace uh::cluster
