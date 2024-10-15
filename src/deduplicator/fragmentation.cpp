#include "fragmentation.h"

#include "common/debug/debug.h"
#include "config.h"

#include <optional>

namespace uh::cluster {

fragmentation::fragmentation(dedupe_logger& dd_logger)
    : m_dedupe_logger(dd_logger),
      m_effective_size(0ull),
      m_unstored_size(0ull) {}

void fragmentation::push_stored(uint128_t pointer, size_t size,
                                std::string_view data, bool header) {
    if (data.empty()) {
        return;
    }
    dd_fragment frag = {
        .type = STORED,
        .data = data,
        .header = header,
        .pointer = pointer,
        .size = size,
    };
    m_frags.emplace_back(std::move(frag));
}

void fragmentation::push_unstored(
    std::string_view data, bool header,
    std::optional<fragment_set::hint_type>&& hint) {
    dd_fragment frag = {
        .type = UNSTORED,
        .data = data,
        .header = header,
        .size = data.size(),
        .hint = std::move(hint),
    };
    m_frags.emplace_back(std::move(frag));
    m_effective_size += data.size();
    m_unstored_size += data.size();
}

void fragmentation::flush_fragment_set(fragment_set& set) {
    if (m_unstored_size == 0ull) {
        return;
    }

    flush_fragments_internal(set);
    mark_as_uploaded();
}

std::size_t fragmentation::effective_size() const { return m_effective_size; }
std::size_t fragmentation::unstored_size() const { return m_unstored_size; }

address fragmentation::make_address() const {
    if (m_unstored_size != 0ull) {
        throw std::runtime_error("fragmentation must be flushed first");
    }

    address rv;

    for (const auto& frag : m_frags) {
        if (frag.type == UNSTORED) {
            rv.append(frag.addr);
            continue;
        }

        if (frag.type == STORED) {
            rv.push({frag.pointer, frag.size});
            continue;
        }
    }

    return rv;
}

address fragmentation::get_stored_fragments() const {
    address rv;

    for (const auto& frag : m_frags) {
        if (frag.type == STORED) {
            rv.push({frag.pointer, frag.size});
        }
    }

    return rv;
}

coro<void> fragmentation::flush_storage(context& ctx, global_data_view& gdv) {
    if (m_unstored_size == 0ull) {
        co_return;
    }

    auto buffer = unstored_to_buffer();
    m_buffer_address = co_await gdv.write(ctx, {&buffer[0], buffer.size()});

    compute_unstored_addresses();
    co_await link_unstored_fragments(ctx, gdv);
}

coro<void> fragmentation::link_unstored_fragments(context& ctx,
                                                  global_data_view& gdv) {
    address unstored;
    for (const auto& frag : m_frags) {
        if (frag.type == UNSTORED) {
            unstored.append(frag.addr);
        }
    }

    co_await gdv.link(ctx, unstored);

    std::size_t freed_bytes = co_await gdv.unlink(ctx, m_buffer_address);
    if (freed_bytes != 0) {
        throw std::runtime_error("there is a mismatch between the stored "
                                 "address and computed addresses");
    }
}

void fragmentation::flush_fragments_internal(fragment_set& set) {

    auto lock = set.lock();
    LOG_CORO_CONTEXT();

    for (auto& frag : m_frags) {
        if (frag.type == STORED) {
            set.mark_deduplication({frag.pointer, frag.size});
            continue;
        }

        m_dedupe_logger.log_non_deduplication(frag.addr.get(0));

        set.insert({frag.addr.pointers[0], frag.addr.pointers[1]},
                   frag.data.substr(0, frag.addr.sizes.front()), frag.header,
                   frag.hint);
    }
}

void fragmentation::mark_as_uploaded() { m_unstored_size = 0ull; }

void fragmentation::compute_unstored_addresses() {
    std::optional<fragment> current;
    std::size_t current_ofs = 0ull;
    std::size_t current_idx = 0ull;

    for (auto& frag : m_frags) {
        if (frag.type != UNSTORED) {
            continue;
        }

        std::size_t frag_offs = 0ull;

        while (frag_offs < frag.data.size()) {

            if (!current || current_ofs >= current->size) {
                if (current_idx >= m_buffer_address.size()) {
                    throw std::runtime_error("insufficient data");
                }

                current = m_buffer_address.get(current_idx);
                current_ofs = 0ull;
                ++current_idx;
            }

            auto size = std::min(current->size - current_ofs,
                                 frag.data.size() - frag_offs);

            frag.addr.push(fragment{current->pointer + current_ofs, size});
            frag_offs += size;
            current_ofs += size;
        }
    }
}

unique_buffer<char> fragmentation::unstored_to_buffer() {
    unique_buffer<char> buffer(m_unstored_size);
    std::size_t offs = 0ull;

    for (auto& frag : m_frags) {
        if (frag.type != UNSTORED) {
            continue;
        }

        memcpy(&buffer[offs], frag.data.data(), frag.data.size());
        offs += frag.data.size();
    }

    return buffer;
}

void fragmentation::handle_rejected_fragments(const address& addr,
                                              fragment_set& set) {
    std::size_t last_frag_pos = 0;
    for (auto& frag : m_frags) {
        if (frag.type != STORED) {
            continue;
        }

        for (size_t i = last_frag_pos; i < addr.size(); i++) {
            auto rejected_frag = addr.get(i);
            if (rejected_frag.pointer == frag.pointer &&
                rejected_frag.size == frag.size) {
                set.erase({rejected_frag.pointer, rejected_frag.size},
                          frag.header);
                frag.type = UNSTORED;
                frag.pointer = 0;
                frag.hint = std::nullopt;
                last_frag_pos = i + 1;

                m_effective_size += frag.data.size();
                m_unstored_size += frag.data.size();
                break;
            }
        }
    }
}

} // namespace uh::cluster
