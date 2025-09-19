/*
 * HTTP writer/reader bodies which supports get/put API only
 */
#pragma once

#include <proxy/cache/asio.h>

#include <proxy/cache/disk/object.h>
#include <proxy/cache/disk/utils.h>

#include <common/crypto/hash.h>
#include <common/types/common_types.h>
#include <common/utils/strings.h>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>
#include <storage/interfaces/data_view.h>

namespace uh::cluster::proxy::cache::disk {

class writer {
public:
    writer(storage::data_view& writer)
        : m_storage{writer},
          m_addr{} {}

    template <typename T> coro<void> put(const T& s) {
        return put(std::span<const char>(s.data(), s.size()));
    }

    coro<void> put(std::span<const char> sv) {
        if (sv.size() == 0) {
            co_return;
        }
        auto addr = co_await m_storage.write(sv, {0});
        m_hash.consume(sv);
        m_addr.append(addr);
    }

    void set_header_size(std::size_t size) { m_header_size = size; }
    /*
     * Moves and returns the internal resource.
     * May only be called once; further calls will return an empty or invalid
     * value.
     */
    object_handle get_object_handle() {
        // TODO: set etag with `to_hex(m_hash.finalize())`
        return object_handle(std::move(m_addr), m_header_size);
    }

private:
    storage::data_view& m_storage;

    md5 m_hash;
    address m_addr;
    std::size_t m_header_size{0};
};

class reader {
public:
    reader(storage::data_view& storage, std::shared_ptr<object_handle> objh)
        : m_storage(storage),
          m_objh{std::move(objh)} {}

    reader(const reader&) = delete;
    reader& operator=(const reader&) = delete;
    reader(reader&&) = delete;
    reader& operator=(reader&&) = delete;

    std::size_t get_header_size() const { return m_objh->header_size(); }

    template <typename T> coro<std::span<const char>> get(T& s) {
        return get(std::span<char>(s.data(), s.size()));
    }

    coro<std::span<const char>> get(std::span<char> buffer) {
        std::size_t read_size = 0;
        address partial_addr;
        while (m_addr_index < m_objh->get_address().size() &&
               read_size < buffer.size()) {

            auto frag = m_objh->get_address().get(m_addr_index);
            if (m_frag_offset > 0) {
                frag.pointer += m_frag_offset;
                frag.size -= m_frag_offset;
            }
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
            co_await m_storage.read_address(partial_addr,
                                            {buffer.data(), read_size});
        }
        co_return std::span<const char>{buffer.data(), read_size};
    }

private:
    storage::data_view& m_storage;
    std::shared_ptr<object_handle> m_objh;

    std::size_t m_addr_index{0};
    std::size_t m_frag_offset{0};
};

} // namespace uh::cluster::proxy::cache::disk
