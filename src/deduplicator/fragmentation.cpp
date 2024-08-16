#include "fragmentation.h"

#include "config.h"
#include <optional>

namespace uh::cluster {

fragmentation::fragmentation(dedupe_logger& dd_logger)
    : m_dedupe_logger(dd_logger),
      m_effective_size(0ull),
      m_unstored_size(0ull) {}

void fragmentation::push(stored&& st) { m_frags.emplace_back(std::move(st)); }

void fragmentation::push(unstored&& un) {
    m_frags.emplace_back(std::move(un));
    m_effective_size += un.data.size();
    m_unstored_size += un.data.size();
}

void fragmentation::flush_set(fragment_set& set) {
    if (m_unstored_size == 0ull) {
        return;
    }

    flush_fragments(set);
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
        if (std::holds_alternative<unstored>(frag)) {
            const auto& un = std::get<unstored>(frag);
            rv.append(un.addr);
            continue;
        }

        if (std::holds_alternative<stored>(frag)) {
            auto stored_frag = std::get<stored>(frag);
            rv.push({stored_frag.pointer, stored_frag.size});
            continue;
        }
    }

    return rv;
}

address fragmentation::get_stored_fragments() const {
    address rv;

    for (const auto& frag : m_frags) {
        if (std::holds_alternative<stored>(frag)) {
            auto stored_frag = std::get<stored>(frag);
            rv.push({stored_frag.pointer, stored_frag.size});
            continue;
        }
    }

    return rv;
}

coro<void> fragmentation::flush_data(context& ctx, global_data_view& gdv) {
    if (m_unstored_size == 0ull) {
        co_return;
    }

    auto buffer = unstored_to_buffer();

    auto addr = co_await gdv.write(ctx, {&buffer[0], buffer.size()});

    compute_unstored_addresses(addr);
}

void fragmentation::flush_fragments(fragment_set& set) {

    auto lock = set.lock();
    LOG_CORO_CONTEXT();

    for (auto& frag : m_frags) {
        if (!std::holds_alternative<unstored>(frag)) {
            auto stored_frag = std::get<stored>(frag);
            set.mark_deduplication({stored_frag.pointer, stored_frag.size});
            continue;
        }

        auto& un = std::get<unstored>(frag);
        if (un.uploaded) {
            continue;
        }

        m_dedupe_logger.log_non_deduplication(un.addr.get(0));

        set.insert({un.addr.pointers[0], un.addr.pointers[1]},
                   un.data.substr(0, un.addr.sizes.front()), un.header,
                   un.hint);
    }
}

void fragmentation::mark_as_uploaded() {
    for (auto& frag : m_frags) {
        if (!std::holds_alternative<unstored>(frag)) {
            continue;
        }

        auto& un = std::get<unstored>(frag);
        un.uploaded = true;
    }

    m_unstored_size = 0ull;
}

void fragmentation::compute_unstored_addresses(const address& addr) {
    std::optional<fragment> current;
    std::size_t current_ofs = 0ull;
    std::size_t current_idx = 0ull;

    for (auto& frag : m_frags) {
        if (!std::holds_alternative<unstored>(frag)) {
            continue;
        }

        auto& un = std::get<unstored>(frag);
        if (un.uploaded) {
            continue;
        }

        std::size_t un_offs = 0ull;

        while (un_offs < un.data.size()) {

            if (!current || current_ofs >= current->size) {
                if (current_idx >= addr.size()) {
                    throw std::runtime_error("insufficient data");
                }
                current = addr.get(current_idx);
                current_ofs = 0ull;
                ++current_idx;
            }

            auto size =
                std::min(current->size - current_ofs, un.data.size() - un_offs);

            un.addr.push(fragment{current->pointer + current_ofs, size});
            un_offs += size;
            current_ofs += size;
        }
    }
}

unique_buffer<char> fragmentation::unstored_to_buffer() {
    unique_buffer<char> buffer(m_unstored_size);
    std::size_t offs = 0ull;

    for (auto& frag : m_frags) {
        if (!std::holds_alternative<unstored>(frag)) {
            continue;
        }

        auto& un = std::get<unstored>(frag);
        if (un.uploaded) {
            continue;
        }

        memcpy(&buffer[offs], un.data.data(), un.data.size());
        offs += un.data.size();
    }

    return buffer;
}
void fragmentation::handle_rejected_fragments(const address& addr,
                                              fragment_set& set) {
    for (auto it = m_frags.begin(); it != m_frags.end(); it++) {
        if (!std::holds_alternative<stored>(*it)) {
            continue;
        }

        auto& stored_frag = std::get<stored>(*it);

        for (std::size_t i = 0; i < addr.size(); i++) {
            auto rejected_frag = addr.get(i);
            if (rejected_frag.pointer == stored_frag.pointer) {
                set.erase({rejected_frag.pointer, rejected_frag.size});
                it->emplace<1>(unstored{.data = stored_frag.data,
                                        .header = stored_frag.header,
                                        .hint = std::nullopt});
                break;
            }
        }
    }
}

} // namespace uh::cluster
