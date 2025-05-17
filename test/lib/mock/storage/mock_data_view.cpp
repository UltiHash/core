#include "mock_data_view.h"

#include <storage/group/impl/address_utils.h>

namespace uh::cluster {
mock_data_view::mock_data_view(mock_data_store& storage)
    : m_storage{storage} {}

coro<address> mock_data_view::write(std::span<const char> data,
                                    const std::vector<std::size_t>& offsets) {
    auto alloc = m_storage.allocate(data.size());
    co_return m_storage.write(alloc, data, offsets);
}

coro<shared_buffer<>> mock_data_view::read(const uint128_t& pointer,
                                           size_t size) {
    shared_buffer<char> buffer(size);
    m_storage.read(pointer, buffer.span());
    co_return buffer;
}

coro<std::size_t> mock_data_view::read_address(const address& addr,
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

coro<std::size_t> mock_data_view::get_used_space() {
    co_return m_storage.get_used_space();
}

[[nodiscard]] coro<address> mock_data_view::link(const address& addr) {
    co_return m_storage.link(addr);
}

coro<std::size_t> mock_data_view::unlink(const address& addr) {
    co_return m_storage.unlink(addr);
}

} // namespace uh::cluster
