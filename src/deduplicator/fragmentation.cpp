#include "fragmentation.h"

#include "config.h"

#include <optional>

namespace uh::cluster {

fragmentation::fragmentation()
    : m_effective_size(0ull),
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
    if (m_unstored_size == 0ull) {
        return;
    }

    flush_fragments_internal(set);
    mark_as_uploaded();
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

coro<void> fragmentation::flush_storage(storage::data_view& gdv) {
    if (m_unstored_size == 0ull) {
        co_return;
    }

    auto buffer = unstored_to_buffer();
    m_buffer_address =
        co_await gdv.write({&buffer[0], buffer.size()}, m_offsets);

    compute_unstored_addresses();
}

void fragmentation::flush_fragments_internal(fragment_set& set) {

    auto lock = set.lock();

    for (auto& frag : m_frags) {
        if (frag.type == STORED) {
            set.mark_deduplication({frag.stored_pointer, frag.stored_size});
            continue;
        }

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
    m_offsets.reserve(m_frags.size());
    std::size_t offs = 0ull;

    for (auto& frag : m_frags) {
        if (frag.type != UNSTORED) {
            continue;
        }
        m_offsets.push_back(offs);
        memcpy(&buffer[offs], frag.data.data(), frag.data.size());
        offs += frag.data.size();
    }

    return buffer;
}

void fragmentation::handle_rejected_fragments(const address& addr,
                                              fragment_set& set) {
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
}

} // namespace uh::cluster
