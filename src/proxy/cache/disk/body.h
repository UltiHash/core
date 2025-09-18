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

class reader_body {
public:
    reader_body(storage::data_view& writer)
        : m_storage{writer},
          m_addr{} {}

    coro<std::size_t> put(std::span<const char> sv) {

        auto addr = co_await m_storage.write(sv, {0});
        m_hash.consume(sv);
        m_addr.append(addr);
        co_return addr.data_size();
    }

    /*
     * Moves and returns the internal resource.
     * May only be called once; further calls will return an empty or invalid
     * value.
     */
    object_handle get_object_handle() {
        // TODO: set etag with `to_hex(m_hash.finalize())`
        return object_handle(std::move(m_addr));
    }

private:
    storage::data_view& m_storage;

    md5 m_hash;
    address m_addr;
};

class writer_body {
public:
    using support_double_buffer = std::false_type;

    writer_body(storage::data_view& storage,
                std::shared_ptr<object_handle> objh, std::size_t buffer_size)
        : m_storage(storage),
          m_objh{std::move(objh)},
          m_buffer(buffer_size) {}

    writer_body(const writer_body&) = delete;
    writer_body& operator=(const writer_body&) = delete;
    writer_body(writer_body&&) = delete;
    writer_body& operator=(writer_body&&) = delete;

    coro<std::span<const char>> get() { co_return co_await _get(&m_buffer); }

private:
    storage::data_view& m_storage;
    std::shared_ptr<object_handle> m_objh;

    std::size_t m_addr_index{0};
    std::size_t m_frag_offset{0};

protected:
    std::vector<char> m_buffer;

    coro<std::span<const char>> _get(std::vector<char>* buffer) {
        std::size_t read_size = 0;

        address partial_addr;
        while (m_addr_index < m_objh->get_address().size() &&
               read_size < buffer->size()) {

            auto frag = m_objh->get_address().get(m_addr_index);
            if (m_frag_offset > 0) {
                frag.pointer += m_frag_offset;
                frag.size -= m_frag_offset;
            }
            if (frag.size + read_size > buffer->size()) {
                auto remains = buffer->size() - read_size;
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
                                            {buffer->data(), read_size});
        }
        co_return std::span<const char>{buffer->data(), read_size};
    }
};

class double_buffered_writer_body : private writer_body {
public:
    using support_double_buffer = std::true_type;

    double_buffered_writer_body(storage::data_view& storage,
                                std::shared_ptr<object_handle> objh,
                                std::size_t buffer_size = 16_MiB)
        : writer_body(storage, objh, buffer_size),
          m_buffer2(buffer_size),
          m_active(&m_buffer),
          m_standby(&m_buffer2) {}

    double_buffered_writer_body(const double_buffered_writer_body&) = delete;
    double_buffered_writer_body&
    operator=(const double_buffered_writer_body&) = delete;
    double_buffered_writer_body(double_buffered_writer_body&&) = delete;
    double_buffered_writer_body&
    operator=(double_buffered_writer_body&&) = delete;

    coro<std::span<const char>> get() {
        auto rv = co_await writer_body::_get(m_active);
        std::swap(m_active, m_standby);
        co_return rv;
    }

private:
    std::vector<char> m_buffer2;
    std::vector<char>* m_active;
    std::vector<char>* m_standby;
};
} // namespace uh::cluster::proxy::cache::disk
