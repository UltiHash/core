#include "fragmentation.h"

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
        .stored_pointer = pointer,
        .stored_size = size,
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
        .stored_size = data.size(),
        .hint = std::move(hint),
    };
    m_frags.emplace_back(std::move(frag));
    m_effective_size += data.size();
    m_unstored_size += data.size();
}

void fragmentation::flush_fragment_set(fragment_set& set) {
    if (m_state != FLUSHED_STORAGE) {
        throw std::runtime_error("flush_fragment_set may only be called "
                                 "after calling flush_storage");
    }

    if (m_unstored_size == 0ull) {
        m_state = FLUSHED_FRAGMENT_SET;
        return;
    }

    flush_fragments_internal(set);
    mark_as_uploaded();

    m_state = FLUSHED_FRAGMENT_SET;
}

std::size_t fragmentation::effective_size() const { return m_effective_size; }
std::size_t fragmentation::unstored_size() const { return m_unstored_size; }

void fragmentation::merge_consecutive_unstored() {
    auto it = m_frags.begin();
    while (it != m_frags.end()) {
        if (it->type == UNSTORED) {
            auto next_it = std::next(it);
            while (next_it != m_frags.end() && next_it->type == UNSTORED &&
                   it->addr.consecutive(next_it->addr)) {
                it->addr.append(next_it->addr);
                next_it = m_frags.erase(next_it);
            }
            it->addr = it->addr.shrink();
        }
        ++it;
    }
};

address fragmentation::make_address() const {
    if (m_state != MERGED_AND_LINKED_UNSTORED or m_unstored_size != 0ull) {
        throw std::runtime_error("make_address may only be called "
                                 "after calling merge_and_link_unstored");
    }

    address rv;

    for (const auto& frag : m_frags) {
        if (frag.type == UNSTORED) {
            rv.append(frag.addr);
            continue;
        }

        if (frag.type == STORED) {
            rv.push({frag.stored_pointer, frag.stored_size});
            continue;
        }
    }

    return rv;
}

address fragmentation::get_stored_fragments() const {
    address rv;

    for (const auto& frag : m_frags) {
        if (frag.type == STORED) {
            rv.push({frag.stored_pointer, frag.stored_size});
        }
    }

    return rv;
}

coro<void> fragmentation::flush_storage(context& ctx, global_data_view& gdv) {
    if (!(m_state == DEDUPE_IN_PROGRESS or m_state == HANDLED_REJECTED)) {
        throw std::runtime_error("flush_storage may only be called "
                                 "after calling handle_rejected_fragments");
    }

    if (m_unstored_size == 0ull) {
        m_state = FLUSHED_STORAGE;
        co_return;
    }

    auto buffer = unstored_to_buffer();
    m_buffer_address = co_await gdv.write(ctx, {&buffer[0], buffer.size()});

    compute_unstored_addresses();

    m_state = FLUSHED_STORAGE;
}

coro<void> fragmentation::merge_and_link_unstored(context& ctx,
                                                  global_data_view& gdv) {
    if (m_state != FLUSHED_FRAGMENT_SET) {
        throw std::runtime_error("merge_and_link_unstored may only be called "
                                 "after calling flush_fragment_set");
    }

    if (m_unstored_size != 0ull) {
        throw std::runtime_error("fragmentation must be flushed first");
    }

    merge_consecutive_unstored();

    address unstored;
    for (const auto& frag : m_frags) {
        if (frag.type == UNSTORED) {
            unstored.append(frag.addr);
        }
    }

    co_await gdv.link(ctx, unstored);

    std::size_t freed_bytes = co_await gdv.unlink(ctx, m_buffer_address);
    if (freed_bytes != 0) {
        LOG_WARN() << ctx.peer() << ": unexpectedly freed " << freed_bytes
                   << " bytes, "
                   << "unstored=" << unstored.to_string()
                   << ", m_buffer_address=" << m_buffer_address.to_string();
        throw std::runtime_error("there is a mismatch between the stored "
                                 "address and computed addresses");
    }

    m_state = MERGED_AND_LINKED_UNSTORED;
}

void fragmentation::flush_fragments_internal(fragment_set& set) {

    auto lock = set.lock();

    for (auto& frag : m_frags) {
        if (frag.type == STORED) {
            set.mark_deduplication({frag.stored_pointer, frag.stored_size});
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
    if (m_state != DEDUPE_IN_PROGRESS) {
        throw std::runtime_error(
            "handle_rejected_fragments may only be called "
            "after the deduplication process has completed");
    }
    std::size_t last_frag_pos = 0;
    for (auto& stored_frag : m_frags) {
        if (stored_frag.type != STORED) {
            continue;
        }

        for (size_t i = last_frag_pos; i < addr.size(); i++) {
            auto rejected_frag = addr.get(i);
            if (rejected_frag.pointer == stored_frag.stored_pointer &&
                rejected_frag.size == stored_frag.stored_size) {
                set.erase({rejected_frag.pointer, rejected_frag.size},
                          stored_frag.header);
                stored_frag.type = UNSTORED;
                stored_frag.stored_pointer = 0;
                stored_frag.hint = std::nullopt;
                last_frag_pos = i + 1;

                m_effective_size += stored_frag.data.size();
                m_unstored_size += stored_frag.data.size();
                break;
            }
        }
    }
    m_state = HANDLED_REJECTED;
}

} // namespace uh::cluster
