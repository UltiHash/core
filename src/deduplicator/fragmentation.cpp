#include "fragmentation.h"

#include <optional>

namespace uh::cluster {

fragmentation::fragmentation(dedupe_logger& dd_logger)
    : m_dedupe_logger(dd_logger),
      m_effective_size(0ull),
      m_unstored_size(0ull) {}

void fragmentation::push(const fragment& f) { m_frags.emplace_back(f); }

void fragmentation::push(unstored&& un) {
    m_frags.emplace_back(std::move(un));
    m_effective_size += un.data.size();
    m_unstored_size += un.data.size();
}

void fragmentation::flush(global_data_view& gdv, fragment_set& set) {
    if (m_unstored_size == 0ull) {
        return;
    }

    flush_data(gdv);
    flush_fragments(gdv, set);
    mark_as_uploaded();
}

std::size_t fragmentation::effective_size() const { return m_effective_size; }
std::size_t fragmentation::unstored_size() const { return m_unstored_size; }

address fragmentation::make_address() const {
    if (m_unstored_size != 0ull) {
        throw std::runtime_error("fragmentation must be flushed first");
    }

    address rv;

    for (const auto& m_frag : m_frags) {
        if (std::holds_alternative<unstored>(m_frag)) {
            const auto& un = std::get<unstored>(m_frag);
            rv.append_address(un.addr);
            continue;
        }

        if (std::holds_alternative<fragment>(m_frag)) {
            rv.push_fragment(std::get<fragment>(m_frag));
            continue;
        }
    }

    return rv;
}

void fragmentation::flush_data(global_data_view& gdv) {
    auto buffer = unstored_to_buffer();

    auto addr = gdv.write({&buffer[0], buffer.size()});

    compute_unstored_addresses(addr);
}

void fragmentation::flush_fragments(global_data_view& gdv, fragment_set& set) {
    if (m_unstored_size == 0ull) {
        return;
    }

    auto lock = set.lock();
    for (auto& m_frag : m_frags) {
        if (!std::holds_alternative<unstored>(m_frag)) {
            set.mark_deduplication(std::get<fragment>(m_frag));
            continue;
        }

        auto& un = std::get<unstored>(m_frag);
        if (un.uploaded) {
            continue;
        }

        m_dedupe_logger.log_non_deduplication(un.addr.get_fragment(0));

        set.insert({un.addr.pointers[0], un.addr.pointers[1]},
                   un.data.substr(0, un.addr.sizes.front()), un.hint);
    }
}

void fragmentation::mark_as_uploaded() {
    for (auto& m_frag : m_frags) {
        if (!std::holds_alternative<unstored>(m_frag)) {
            continue;
        }

        auto& un = std::get<unstored>(m_frag);
        un.uploaded = true;
    }

    m_unstored_size = 0ull;
}

void fragmentation::compute_unstored_addresses(const address& addr) {
    std::optional<fragment> current;
    std::size_t current_ofs = 0ull;
    std::size_t current_idx = 0ull;

    for (auto& m_frag : m_frags) {
        if (!std::holds_alternative<unstored>(m_frag)) {
            continue;
        }

        auto& un = std::get<unstored>(m_frag);
        if (un.uploaded) {
            continue;
        }

        std::size_t un_offs = 0ull;

        while (un_offs < un.data.size()) {

            if (!current || current_ofs >= current->size) {
                if (current_idx >= addr.size()) {
                    throw std::runtime_error("insufficient data");
                }
                current = addr.get_fragment(current_idx);
                current_ofs = 0ull;
                ++current_idx;
            }

            auto size =
                std::min(current->size - current_ofs, un.data.size() - un_offs);

            un.addr.push_fragment(
                fragment{current->pointer + current_ofs, size});
            un_offs += size;
            current_ofs += size;
        }
    }
}

unique_buffer<char> fragmentation::unstored_to_buffer() {
    unique_buffer<char> buffer(m_unstored_size);
    std::size_t offs = 0ull;

    for (auto& m_frag : m_frags) {
        if (!std::holds_alternative<unstored>(m_frag)) {
            continue;
        }

        auto& un = std::get<unstored>(m_frag);
        if (un.uploaded) {
            continue;
        }

        memcpy(&buffer[offs], un.data.data(), un.data.size());
        offs += un.data.size();
    }

    return buffer;
}

} // namespace uh::cluster
