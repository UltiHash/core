/*
 * Sync/source for disk, which supports put/get API
 */
#pragma once

#include <proxy/cache/disk/object.h>
#include <proxy/cache/disk/utils.h>

#include <common/crypto/hash.h>
#include <common/types/common_types.h>
#include <common/utils/strings.h>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>
#include <storage/interfaces/data_view.h>

namespace uh::cluster::proxy::cache::disk {

class disk_sink {
public:
    disk_sink(storage::data_view& writer)
        : m_storage{writer},
          m_addr{} {}

    disk_sink(const disk_sink&) = delete;
    disk_sink& operator=(const disk_sink&) = delete;
    disk_sink(disk_sink&&) = default;
    disk_sink& operator=(disk_sink&&) = default;

    coro<void> put(std::span<const char> sv) {
        if (sv.size() == 0) {
            co_return;
        }
        auto addr = co_await m_storage.get().write(sv, {0});
        m_addr.append(addr);
    }

    void set_header_size(std::size_t size) { m_header_size = size; }
    /*
     * Moves and returns the internal resource.
     * May only be called once; further calls will return an empty or invalid
     * value.
     */
    object_handle get_object_handle() {
        return object_handle(std::move(m_addr), m_header_size);
    }

private:
    std::reference_wrapper<storage::data_view> m_storage;

    address m_addr;
    std::size_t m_header_size{0};
};

class disk_source {
public:
    disk_source(storage::data_view& storage,
                std::shared_ptr<object_handle> objh)
        : m_storage(storage),
          m_objh{std::move(objh)} {}

    disk_source(const disk_source&) = delete;
    disk_source& operator=(const disk_source&) = delete;
    disk_source(disk_source&&) = default;
    disk_source& operator=(disk_source&&) = default;

    std::size_t get_header_size() const { return m_objh->header_size(); }

    coro<std::span<const char>> get(std::span<char> buffer) {
        std::size_t read_size = 0;
        address partial_addr;
        while (m_addr_index < m_objh->get_address().size() &&
               read_size < buffer.size()) {

            auto frag = m_objh->get_address().get(m_addr_index);

            frag.pointer += m_frag_offset;
            frag.size -= m_frag_offset;

            if (frag.size + read_size > buffer.size()) {
                auto remains = buffer.size() - read_size;
                m_frag_offset += remains;
                frag.size = remains;
                partial_addr.push(frag);
            } else {
                m_frag_offset = 0;
                partial_addr.push(frag);
                m_addr_index++;
            }

            read_size += frag.size;
        }

        if (read_size > 0) {
            co_await m_storage.get().read_address(partial_addr,
                                                  {buffer.data(), read_size});
        }
        co_return std::span<const char>{buffer.data(), read_size};
    }

private:
    std::reference_wrapper<storage::data_view> m_storage;
    std::shared_ptr<object_handle> m_objh;

    std::size_t m_addr_index{0};
    std::size_t m_frag_offset{0};
};

} // namespace uh::cluster::proxy::cache::disk
