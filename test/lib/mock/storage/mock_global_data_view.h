#pragma once

#include <storage/global_data/global_data_view.h>

#include "mock_data_store.h"

namespace uh::cluster {

class mock_global_data_view : public global_data_view {
public:
    mock_global_data_view(mock_data_store& storage);

    coro<address> write(std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override;
    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size) override;
    shared_buffer<char> read_fragment(const uint128_t& pointer,
                                      size_t size) override;
    coro<std::size_t> read_address(const address& addr,
                                   std::span<char> buffer) override;
    [[nodiscard]] coro<address> link(const address& addr) override;
    coro<std::size_t> unlink(const address& addr) override;
    coro<std::size_t> get_used_space() override;

    ~mock_global_data_view() noexcept = default;

private:
    mock_data_store& m_storage;
};

} // namespace uh::cluster
