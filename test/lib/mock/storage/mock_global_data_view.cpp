#include "mock_global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
mock_global_data_view::mock_global_data_view(mock_data_store& storage)
    : m_storage{storage} {}

coro<address>
mock_global_data_view::write(context& ctx, std::span<const char> data,
                             const std::vector<std::size_t>& offsets) {
    co_return m_storage.write(data, offsets);
}

shared_buffer<char>
mock_global_data_view::read_fragment(context& ctx, const uint128_t& pointer,
                                     const size_t size) {
    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }
    shared_buffer<char> buffer(size);
    m_storage.read(pointer, buffer.span());
    return buffer;
}

coro<shared_buffer<>> mock_global_data_view::read(context& ctx,
                                                  const uint128_t& pointer,
                                                  size_t size) {
    shared_buffer<char> buffer(size);
    m_storage.read(pointer, buffer.span());
    co_return buffer;
}

coro<std::size_t> mock_global_data_view::read_address(context& ctx,
                                                      const address& addr,
                                                      std::span<char> buffer) {
    auto size = 0u;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        m_storage.read(frag.pointer, buffer.first(frag.size));
        buffer = buffer.subspan(frag.size);
        size += frag.size;
    }

    co_return size;
}

coro<std::size_t> mock_global_data_view::get_used_space(context& ctx) {
    co_return m_storage.get_used_space();
}

[[nodiscard]] coro<address> mock_global_data_view::link(context& ctx,
                                                        const address& addr) {
    co_return m_storage.link(addr);
}

coro<std::size_t> mock_global_data_view::unlink(context& ctx,
                                                const address& addr) {
    co_return m_storage.unlink(addr);
}

} // namespace uh::cluster
