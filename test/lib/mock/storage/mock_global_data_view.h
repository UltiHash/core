#pragma once

#include <storage/interface.h>

#include "mock_data_store.h"

namespace uh::cluster {

class mock_global_data_view : public sn::interface {
public:
    mock_global_data_view(mock_data_store& storage);

    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override;

    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char>) override;

    [[nodiscard]] coro<address> link(context& ctx,
                                     const address& addr) override;
    coro<std::size_t> unlink(context& ctx, const address& addr) override;
    coro<std::size_t> get_used_space(context& ctx) override;

    ~mock_global_data_view() noexcept = default;

private:
    mock_data_store& m_storage;
};

} // namespace uh::cluster
