#pragma once

#include "common/global_data/global_data_view.h"

#include "fake_data_store.h"

namespace uh::cluster {

class fake_global_data_view : public global_data_view {
public:
    fake_global_data_view(boost::asio::io_context& ioc,
                          fake_data_store& storage);

    coro<address> write(context& ctx, const std::string_view& data,
                        const std::vector<std::size_t>& offsets) override;
    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size) override;
    shared_buffer<char> read_fragment(context& ctx, const uint128_t& pointer,
                                      size_t size) override;
    coro<std::size_t> read_address(context& ctx, char* buffer,
                                   const address& addr) override;
    [[nodiscard]] coro<address> link(context& ctx,
                                     const address& addr) override;
    coro<std::size_t> unlink(context& ctx, const address& addr) override;
    coro<std::size_t> get_used_space(context& ctx) override;
    [[nodiscard]] boost::asio::io_context& get_executor() const override;
    [[nodiscard]] std::size_t
    get_storage_service_connection_count() const noexcept override;

    ~fake_global_data_view() noexcept = default;

private:
    boost::asio::io_context& m_ioc;
    fake_data_store& m_storage;
};

} // namespace uh::cluster
