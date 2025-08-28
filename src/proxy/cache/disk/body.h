#pragma once

#include <proxy/cache/body.h>

#include <proxy/cache/disk/object.h>
#include <proxy/cache/disk/utils.h>

#include <common/crypto/hash.h>
#include <common/utils/strings.h>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>

namespace uh::cluster::proxy::cache::disk {

class reader_body {
public:
    reader_body(deduplicator_interface& writer)
        : m_dedupe{writer},
          m_object_handle(std::make_shared<object_handle>()) {}

    coro<std::size_t> put(std::span<const char> sv) {
        auto objh = co_await utils::create(m_dedupe, sv);
        m_hash.consume(sv);
        m_object_handle->append(objh);
        co_return objh.size();
    }

    /*
     * Moves and returns the internal resource.
     * May only be called once; further calls will return an empty or invalid
     * value.
     */
    std::shared_ptr<object_handle> get_object_handle() {
        // TODO: set etag with `to_hex(m_hash.finalize())`
        return std::move(m_object_handle);
    }

private:
    deduplicator_interface& m_dedupe;

    md5 m_hash;
    std::shared_ptr<object_handle> m_object_handle;
};

class writer_body {
public:
    writer_body(storage::global::global_data_view& storage,
                std::shared_ptr<object_handle> objh,
                std::size_t buffer_size = 32 * MEBI_BYTE)
        : m_storage(storage),
          m_objh{std::move(objh)},
          m_buffer(buffer_size) {}

    coro<std::span<const char>> get(std::size_t count) {
        if (m_put_ptr - m_get_ptr < count) {
            co_await fill();
        }

        auto size = std::min(count, m_put_ptr - m_get_ptr);
        auto rv = std::span<const char>{&m_buffer[m_get_ptr], size};

        m_get_ptr += size;

        consume();
        co_return rv;
    }

    coro<void> fill() {
        std::size_t count = 0;

        address partial_addr;
        while (m_addr_index < m_objh->addr.size() &&
               count + m_put_ptr < m_buffer.size()) {

            auto frag = m_objh->addr.get(m_addr_index);
            if (m_frag_offset > 0) {
                frag.pointer += m_frag_offset;
                frag.size -= m_frag_offset;
            }

            if (frag.size + count + m_put_ptr > m_buffer.size()) {
                auto remains = m_buffer.size() - (count + m_put_ptr);

                m_frag_offset += remains;
                frag.size = remains;
                partial_addr.push(frag);
                count += frag.size;
                break;
            }

            m_frag_offset = 0;
            partial_addr.push(frag);
            count += frag.size;
            m_addr_index++;
        }

        if (count > 0) {
            LOG_DEBUG() << "local_read_handle: fill, reading " << count
                        << " bytes from storage";
            co_await utils::read(m_storage, *m_objh,
                                 {&m_buffer[m_put_ptr], count});
            m_put_ptr += count;
            m_total += count;
        }
    }

private:
    storage::global::global_data_view& m_storage;
    std::shared_ptr<object_handle> m_objh;

    std::vector<char> m_buffer;
    std::size_t m_get_ptr = 0ull;
    std::size_t m_put_ptr = 0ull;

    size_t m_addr_index = 0;
    std::size_t m_frag_offset = 0;

    std::size_t m_total = 0;

    void consume() {
        auto count = m_put_ptr - m_get_ptr;
        if (count > 0) {
            LOG_DEBUG() << "local_read_handle: copying "
                        << (m_put_ptr - m_get_ptr) << " bytes to new buffer";
            memmove(&m_buffer[0], &m_buffer[m_get_ptr], m_put_ptr - m_get_ptr);
            m_put_ptr -= m_get_ptr;
            m_get_ptr = 0;
        } else {
            m_put_ptr = m_get_ptr = 0ull;
        }
    }
};

} // namespace uh::cluster::proxy::cache::disk
