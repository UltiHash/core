//
// Created by masi on 9/4/23.
//

#ifndef CORE_MESSENGER_H
#define CORE_MESSENGER_H

#include "messenger_core.h"

namespace uh::cluster {

    class messenger: public messenger_core {
    public:
        using messenger_core::messenger_core;

        coro <std::pair <header, address>> recv_address (const header& message_header) {
            address addr;
            addr.allocate_for_serialized_data(message_header.size);
            register_read_buffer(addr.pointers);
            register_read_buffer(addr.sizes);
            co_await recv_buffers(message_header);
            co_return std::pair {message_header, std::move (addr)};
        }

        coro <void> send_address (const message_types type, const address& addr) {
            register_write_buffer (addr.pointers);
            register_write_buffer(addr.sizes);
            co_await send_buffers(type);
        }

        coro <void> send_fragment (const message_types type, const fragment frag) {
            register_write_buffer(frag.pointer.get_data(), 2);
            register_write_buffer(frag.size);
            co_await send_buffers (type);
        }

        coro <std::pair <header, fragment>> recv_fragment (const header& message_header) {
            fragment frag;
            register_read_buffer(frag.pointer.ref_data());
            register_read_buffer(frag.size);
            co_await recv_buffers (message_header);
            co_return std::pair {message_header, frag};
        }

        coro <void> send_uint128_t (const message_types type, const uint128_t num) {
            register_write_buffer(num.get_data(), 2);
            co_await send_buffers (type);
        }

        coro <std::pair <header, uint128_t>> recv_uint128_t (const header& message_header) {
            uint128_t num;
            register_read_buffer(num.ref_data());
            co_await recv_buffers (message_header);
            co_return std::pair {message_header, num};
        }
    };

} // end namespace uh::cluster

#endif //CORE_MESSENGER_H
