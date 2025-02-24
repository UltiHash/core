#pragma once

#include "common/network/messenger.h"
#include <common/telemetry/context.h>
#include <common/utils/protocol_handler.h>
#include <storage/interfaces/local_storage.h>

namespace uh::cluster::storage {

class handler : public protocol_handler {
public:
    explicit handler(local_storage& storage);

    coro<void> handle(boost::asio::ip::tcp::socket s) override;

private:
    coro<void> handle_write(context& ctx, messenger& m,
                            const messenger::header& h);

    coro<void> handle_read(context& ctx, messenger& m,
                           const messenger::header& h);

    coro<void> handle_read_fragment(context& ctx, messenger& m,
                                    const messenger::header& h);

    coro<void> handle_read_address(context& ctx, messenger& m,
                                   const messenger::header& h);

    coro<void> handle_link(context& ctx, messenger& m,
                           const messenger::header& h);

    coro<void> handle_unlink(context& ctx, messenger& m,
                             const messenger::header& h);

    coro<void> handle_get_used(context& ctx, messenger& m,
                               const messenger::header&);

    coro<void> handle_ds_info(context& ctx, messenger& m,
                              const messenger::header&);

    coro<void> handle_init_dd(context& ctx, messenger& m,
                              const messenger::header&);

    coro<void> handle_ds_write(context& ctx, messenger& m,
                               const messenger::header& h);

    coro<void> handle_ds_read(context& ctx, messenger& m,
                              const messenger::header& h);

    local_storage& m_storage;
};

} // namespace uh::cluster::storage
