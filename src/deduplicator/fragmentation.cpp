#include "fragmentation.h"

namespace uh::cluster {

fragmentation::fragmentation()
    : m_effective_size(0ull),
      m_unstored_size(0ull) {}

void fragmentation::push(const fragment& f) { m_frags.push_back(f); }

void fragmentation::push(unstored&& un) {
    m_frags.push_back(std::move(un));
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
    address rv;

    for (auto it = m_frags.begin(); it != m_frags.end(); ++it) {
        if (std::holds_alternative<unstored>(*it)) {
            const unstored& un = std::get<unstored>(*it);
            rv.append_address(un.addr);
            continue;
        }

        if (std::holds_alternative<fragment>(*it)) {
            rv.push_fragment(std::get<fragment>(*it));
            continue;
        }
    }

    return rv;
}

void fragmentation::flush_data(global_data_view& gdv) {
    auto buffer = unstored_to_buffer();

    auto addr = gdv.write({&buffer[0], buffer.size()});

    compute_unstored_addresses(addr);
    gdv.sync(addr);
}

void fragmentation::flush_fragments(global_data_view& gdv, fragment_set& set) {
    if (m_unstored_size == 0ull) {
        return;
    }

    for (auto it = m_frags.begin(); it != m_frags.end(); ++it) {
        if (!std::holds_alternative<unstored>(*it)) {
            continue;
        }

        unstored& un = std::get<unstored>(*it);
        if (un.uploaded) {
            continue;
        }

        set.insert({un.addr.pointers[0], un.addr.pointers[1]},
                   un.data.substr(0, un.addr.sizes.front()), un.hint);
        gdv.add_l1({un.addr.pointers[0], un.addr.pointers[1]},
                   un.data.substr(0, un.addr.sizes.front()));
    }
}

void fragmentation::mark_as_uploaded() {
    for (auto it = m_frags.begin(); it != m_frags.end(); ++it) {
        if (!std::holds_alternative<unstored>(*it)) {
            continue;
        }

        unstored& un = std::get<unstored>(*it);
        un.uploaded = true;
    }

    m_unstored_size = 0ull;
}

void fragmentation::compute_unstored_addresses(const address& addr) {
    fragment current = addr.get_fragment(0);
    std::size_t current_ofs = current.size;
    std::size_t current_idx = 0ull;
    for (auto it = m_frags.begin(); it != m_frags.end(); ++it) {
        if (!std::holds_alternative<unstored>(*it)) {
            continue;
        }

        unstored& un = std::get<unstored>(*it);
        if (un.uploaded) {
            continue;
        }

        std::size_t un_offs = 0ull;

        while (un_offs < un.data.size()) {

            if (current_ofs >= current.size) {
                if (current_idx >= addr.size()) {
                    throw std::runtime_error("insufficient data");
                }
                current = addr.get_fragment(current_idx);
                current_ofs = 0ull;
                ++current_idx;
            }

            auto size =
                std::min(current.size - current_ofs, un.data.size() - un_offs);

            un.addr.push_fragment(
                fragment{current.pointer + current_ofs, size});
            un_offs += size;
            current_ofs += size;
        }
    }
}

std::vector<char> fragmentation::unstored_to_buffer() {
    std::vector<char> buffer(m_unstored_size);
    std::size_t offs = 0ull;

    for (auto it = m_frags.begin(); it != m_frags.end(); ++it) {
        if (!std::holds_alternative<unstored>(*it)) {
            continue;
        }

        unstored& un = std::get<unstored>(*it);
        if (un.uploaded) {
            continue;
        }

        memcpy(&buffer[offs], un.data.data(), un.data.size());
        offs += un.data.size();
    }

    return buffer;
}

} // namespace uh::cluster
