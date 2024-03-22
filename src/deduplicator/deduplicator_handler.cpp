#include "deduplicator_handler.h"

#include "common/utils/common.h"
#include <utility>

namespace uh::cluster {

namespace {

class fragmentation {
public:
    struct unstored {
        std::string_view data;
        std::set<fragment_set_element>::const_iterator hint;
        bool uploaded = false;

        address addr;
    };

    fragmentation(global_data_view& storage)
        : m_storage(storage),
          m_effective_size(0ull),
          m_unstored_size(0ull) {}

    void push(const fragment& f) { m_frags.push_back(f); }

    void push(unstored&& un) {
        m_frags.push_back(std::move(un));
        m_effective_size += un.data.size();
        m_unstored_size += un.data.size();
    }

    /**
     * Convert all unstored fragments to stored fragments. Uploads all frags to
     * downstream storage.
     */
    void flush() {
        if (m_unstored_size == 0ull) {
            return;
        }

        auto buffer = unstored_to_buffer();

        auto addr = m_storage.write({&buffer[0], buffer.size()});

        compute_unstored_addresses(addr);
        m_storage.sync(addr);
    }

    void mark_as_uploaded() {
        for (auto it = m_frags.begin(); it != m_frags.end(); ++it) {
            if (!std::holds_alternative<unstored>(*it)) {
                continue;
            }

            unstored& un = std::get<unstored>(*it);
            un.uploaded = true;
        }

        m_unstored_size = 0ull;
    }

    std::size_t effective_size() const { return m_effective_size; }
    std::size_t unstored_size() const { return m_unstored_size; }

    void flush_fragments(fragment_set& destination) {
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

            destination.insert({un.addr.pointers[0], un.addr.pointers[1]},
                               un.data.substr(0, un.addr.sizes.front()),
                               un.hint);
            m_storage.add_l1({un.addr.pointers[0], un.addr.pointers[1]},
                             un.data.substr(0, un.addr.sizes.front()));
        }

        destination.flush();
    }

    address make_address() const {
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

private:
    void compute_unstored_addresses(const address& addr) {
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

                auto size = std::min(current.size - current_ofs,
                                     un.data.size() - un_offs);

                un.addr.push_fragment(
                    fragment{current.pointer + current_ofs, size});
                un_offs += size;
                current_ofs += size;
            }
        }
    }

    std::vector<char> unstored_to_buffer() {
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

    global_data_view& m_storage;
    std::list<std::variant<fragment, unstored>> m_frags;
    std::size_t m_effective_size;
    std::size_t m_unstored_size;
};

template <typename container>
size_t largest_common_prefix(const container& a, const container& b) noexcept {
    auto mismatch = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    return std::distance(a.begin(), mismatch.first);
}

size_t match_size(global_data_view& storage, std::string_view data, auto frag) {
    if (!frag) {
        return 0ull;
    }

    const fragment_set_element& f = *frag;

    std::size_t common = 0ull;
    auto cached = storage.cached_sample(f.pointer());
    std::size_t common = 0ull;
    if (cached.data()) {
        common = largest_common_prefix(data, cached.get_str_view());
        if (common < storage.l1_cache_sample_size()) {
            return common;
        }
    }

    auto complete = storage.read_fragment(f.pointer(), f.size());
    return largest_common_prefix(data.substr(common),
                                 complete.get_str_view().substr(common));
}

} // namespace

deduplicator_handler::deduplicator_handler(deduplicator_config config,
                                           global_data_view& storage,
                                           worker_pool& dedupe_workers)
    : m_dedupe_conf(std::move(config)),
      m_fragment_set(m_dedupe_conf.working_dir / "log", storage),
      m_storage(storage),
      m_dedupe_workers(dedupe_workers) {
    if (m_dedupe_conf.min_fragment_size > m_storage.l1_cache_sample_size()) {
        throw std::invalid_argument("L1 cache sample size should not be "
                                    "smaller than the min fragment size!");
    }
}

coro<void> deduplicator_handler::handle(boost::asio::ip::tcp::socket s) {
    std::stringstream remote;
    remote << s.remote_endpoint();

    messenger m(std::move(s));

    for (;;) {
        std::optional<error> err;

        try {
            const auto message_header = co_await m.recv_header();

            LOG_DEBUG() << remote.str() << " received "
                        << magic_enum::enum_name(message_header.type);

            switch (message_header.type) {
            case DEDUPLICATOR_REQ:

                co_await handle_dedupe(m, message_header);
                break;
            default:
                throw std::invalid_argument("Invalid message type!");
            }
        } catch (const error_exception& e) {
            err = e.error();
        } catch (const std::exception& e) {
            err = error(error::unknown, e.what());
        }
        if (err) {
            LOG_WARN() << remote.str()
                       << " error handling request: " << err->message();
            co_await m.send_error(*err);
        }
    }
}

coro<void> deduplicator_handler::handle_dedupe(messenger& m,
                                               const messenger::header& h) {

    if (h.size == 0) [[unlikely]] {
        throw std::length_error("Empty data sent do the dedupe node");
    }

    unique_buffer<char> data(h.size);
    m.register_read_buffer(data);
    co_await m.recv_buffers(h);

    const std::size_t pieces_count =
        std::min(m_storage.get_storage_service_connection_count(),
                 static_cast<std::size_t>(std::ceil(
                     static_cast<double>(data.size()) /
                     static_cast<double>(
                         m_dedupe_conf.dedupe_worker_minimum_data_size))));
    size_t piece_size = std::ceil(static_cast<double>(data.size()) /
                                  static_cast<double>(pieces_count));
    std::vector<std::string_view> pieces;
    pieces.reserve(pieces_count);
    for (std::size_t i = 0; i < pieces_count; ++i) {
        pieces.emplace_back(data.get_str_view().substr(
            i * piece_size,
            std::min(piece_size, data.size() - i * piece_size)));
    }

    auto responses =
        co_await m_dedupe_workers.broadcast_from_io_thread_in_workers(
            [this](const auto& piece) { return deduplicate(piece); }, pieces);

    for (std::size_t i = 1; i < pieces_count; i++) {
        responses[0].addr.append_address(responses[i].addr);
        responses[0].effective_size += responses[i].effective_size;
    }

    co_await m.send_dedupe_response(responses[0]);
}

dedupe_response deduplicator_handler::deduplicate(std::string_view data) {

    fragmentation fragments(m_storage);

    while (!data.empty()) {
        const auto f = m_fragment_set.find(data);

        auto match_low = match_size(m_storage, data, f.low);
        auto match_high = match_size(m_storage, data, f.high);

        if (std::max(match_low, match_high) > m_dedupe_conf.min_fragment_size) {

            const fragment_set_element& element =
                match_low > match_high ? *f.low : *f.high;
            std::size_t size = match_low > match_high ? match_low : match_high;

            fragments.push(fragment{element.pointer(), size});
            data = data.substr(size);

            continue;
        }

        auto frag_size = std::min(data.size(), m_dedupe_conf.max_fragment_size);
        fragments.push(
            fragmentation::unstored{data.substr(0, frag_size), f.hint});
        data = data.substr(frag_size);

        if (fragments.unstored_size() >= m_flush_buffer_size) {
            fragments.flush();
            fragments.flush_fragments(m_fragment_set);
            fragments.mark_as_uploaded();
        }
    }

    fragments.flush();
    fragments.flush_fragments(m_fragment_set);
    fragments.mark_as_uploaded();

    dedupe_response result{.effective_size = fragments.effective_size(),
                           .addr = fragments.make_address()};

    return result;
}

} // end namespace uh::cluster
