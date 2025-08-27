#pragma once

#include <common/crypto/hash.h>
#include <common/service_interfaces/deduplicator_interface.h>
#include <common/utils/strings.h>
#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>

namespace uh::cluster::ep::http {

class storage_writer_body : public body {
public:
    storage_writer_body(deduplicator_interface& writer, stream& s,
                        size_t length)
        : m_writer{writer},
          m_s{s},
          m_length{length} {}

    std::optional<std::size_t> length() const { return m_length; }

    coro<std::span<const char>> read(std::size_t count) {
        auto sv = co_await m_s.read(std::min(count, m_length));
        m_dedupe_response.append(co_await m_writer.deduplicate(
            std::string_view{sv.data(), sv.size()}));
        m_hash.consume(sv);
        m_length -= sv.size();
        co_return sv;
    }
    coro<void> consume() { co_await m_s.consume(); }

    std::size_t buffer_size() const { return m_s.buffer_size(); }

    // custom methods
    const dedupe_response& get_dedupe_response() const {
        return m_dedupe_response;
    }
    std::string get_etag() { return to_hex(m_hash.finalize()); }

private:
    deduplicator_interface& m_writer;
    stream& m_s;
    std::size_t m_length;
    md5 m_hash;
    dedupe_response m_dedupe_response;
};

} // namespace uh::cluster::ep::http
