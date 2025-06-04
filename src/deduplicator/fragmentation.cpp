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

        set.insert({frag.addr.fragments[0].pointer},
                   frag.data.substr(0, frag.addr.fragments[0].size),
                   frag.header, frag.hint);
    }
}

void fragmentation::mark_as_uploaded() { m_unstored_size = 0ull; }

void fragmentation::compute_unstored_addresses() {
    std::optional<fragment> storage_frag;
    std::size_t storage_frag_offset = 0ull;
    std::size_t storage_frag_id = 0ull;

    for (auto& dd_frag : m_frags) {
        if (dd_frag.type != UNSTORED) {
            continue;
        }

        std::size_t dd_frag_offset = 0ull;
        while (dd_frag_offset < dd_frag.data.size()) {

            if (!storage_frag || storage_frag_offset == storage_frag->size) {
                if (storage_frag_id >= m_buffer_address.size()) {
                    throw std::runtime_error("insufficient data");
                }

                storage_frag = m_buffer_address.get(storage_frag_id);
                storage_frag_offset = 0ull;
                ++storage_frag_id;
            }

            auto size = std::min(storage_frag->size - storage_frag_offset,
                                 dd_frag.data.size() - dd_frag_offset);

            dd_frag.addr.push(
                fragment{storage_frag->pointer + storage_frag_offset, size});
            dd_frag_offset += size;
            storage_frag_offset += size;

            if (storage_frag_offset > storage_frag->size) {
                throw std::out_of_range("storage fragment overflow");
            }
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
