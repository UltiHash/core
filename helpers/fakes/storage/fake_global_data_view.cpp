#include "fake_global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
fake_global_data_view::fake_global_data_view(boost::asio::io_context& ioc,
                                             fake_data_store& storage)
    : m_ioc{ioc},
      m_storage{storage} {}

coro<address>
fake_global_data_view::write(context& ctx, const std::string_view& data,
                             const std::vector<std::size_t>& offsets) {
    co_return m_storage.write(data, offsets);
}

shared_buffer<char>
fake_global_data_view::read_fragment(context& ctx, const uint128_t& pointer,
                                     const size_t size) {
    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }
    shared_buffer<char> buffer(size);
    m_storage.read(buffer.data(), pointer, size);
    return buffer;
}

coro<shared_buffer<>> fake_global_data_view::read(context& ctx,
                                                  const uint128_t& pointer,
                                                  size_t size) {
    shared_buffer<char> buffer(size);
    m_storage.read(buffer.data(), pointer, size);
    co_return buffer;
}

coro<std::size_t> fake_global_data_view::read_address(context& ctx,
                                                      char* buffer,
                                                      const address& addr) {
    auto size = 0u;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        m_storage.read(buffer, frag.pointer, frag.size);
        buffer += frag.size;
        size += frag.size;
    }

    co_return size;
}

coro<std::size_t> fake_global_data_view::get_used_space(context& ctx) {
    co_return m_storage.get_used_space();
}

[[nodiscard]] coro<address> fake_global_data_view::link(context& ctx,
                                                        const address& addr) {
    co_return m_storage.link(addr);
}

coro<std::size_t> fake_global_data_view::unlink(context& ctx,
                                                const address& addr) {
    co_return m_storage.unlink(addr);
}

[[nodiscard]] boost::asio::io_context&
fake_global_data_view::get_executor() const {
    return m_ioc;
}

[[nodiscard]] std::size_t
fake_global_data_view::get_storage_service_connection_count() const noexcept {
    return 1;
}

} // namespace uh::cluster
