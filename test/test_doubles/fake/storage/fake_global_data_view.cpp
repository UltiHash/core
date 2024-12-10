#include "fake_global_data_view.h"

#include "common/utils/address_utils.h"

namespace uh::cluster {
fake_global_data_view::fake_global_data_view(
    const global_data_view_config& config, boost::asio::io_context& ioc,
    fake_data_store& storage)
    : m_io_service(ioc),
      m_storage(storage) {}

coro<address> fake_global_data_view::write(context& ctx,
                                           const std::string_view& data) {
    co_return 0;
}

shared_buffer<char>
fake_global_data_view::read_fragment(context& ctx, const uint128_t& pointer,
                                     const size_t size) {
    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }
    shared_buffer<char> buffer(size);
    const fragment frag{pointer, size};
    boost::asio::co_spawn(m_io_service,
                          m_storage.read_fragment(ctx, buffer.data(), frag),
                          boost::asio::use_future)
        .get();
    return buffer;
}

coro<shared_buffer<>> fake_global_data_view::read(context& ctx,
                                                  const uint128_t& pointer,
                                                  size_t size) {
    auto buffer = co_await m_storage.read(ctx, pointer, size);
    co_return buffer;
}

coro<std::size_t> fake_global_data_view::read_address(context& ctx,
                                                      char* buffer,
                                                      const address& addr) {
    co_return co_await m_storage.read_address(ctx, buffer, info.addr,
                                              info.pointer_offsets);
}

coro<std::size_t> fake_global_data_view::get_used_space(context& ctx) {
    co_return co_await m_storage.get_used_space(ctx);
}

[[nodiscard]] coro<address> fake_global_data_view::link(context& ctx,
                                                        const address& addr) {
    co_return co_await m_storage.link(ctx, info.addr);
}

coro<std::size_t> fake_global_data_view::unlink(context& ctx,
                                                const address& addr) {
    co_return co_await m_storage.unlink(ctx, info.addr);
}

[[nodiscard]] boost::asio::io_context&
fake_global_data_view::get_executor() const {
    return m_io_service;
}

[[nodiscard]] std::size_t
fake_global_data_view::get_storage_service_connection_count() const noexcept {
    return 1;
}
fake_global_data_view::~fake_global_data_view() noexcept {}

} // namespace uh::cluster
