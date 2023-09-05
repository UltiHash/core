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

        /*
        messenger (boost::asio::io_context& ioc, const std::string& address, const int port):
        messenger_core (ioc, address, port) {}
        explicit messenger (boost::asio::ip::tcp::socket &&socket): messenger_core (std::move (socket)) {}
        messenger (messenger&& m) noexcept : messenger_core(std::move (m)) {}
*/


        coro <std::pair <header, address>> recv_address () {
            const auto message_header = co_await recv_header();
            address addr;
            addr.allocate_for_serialized_data(message_header.size);
            register_read_buffer(addr.pointers);
            register_read_buffer(addr.sizes);
            co_await recv_buffers(message_header);
            co_return std::pair {message_header, std::move (addr)};
        }

        coro <void> send_address (const message_types type, const address& addr) {
            register_write_buffer (addr.pointers);
            register_write_buffer(addr.size());
            co_await send_buffers(type);
        }
    };

} // end namespace uh::cluster

#endif //CORE_MESSENGER_H
