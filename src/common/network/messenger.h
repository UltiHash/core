#ifndef CORE_MESSENGER_H
#define CORE_MESSENGER_H

#include "messenger_core.h"

#include <zpp_bits.h>

namespace uh::cluster {

class messenger : public messenger_core {
public:
    using messenger_core::messenger_core;

    coro<address> recv_address(const header& message_header) {
        address addr;
        addr.allocate_for_serialized_data(message_header.size);
        register_read_buffer(addr.pointers);
        register_read_buffer(addr.sizes);
        co_await recv_buffers(message_header);
        co_return std::move(addr);
    }

    coro<fragment> recv_fragment(const header& message_header) {
        fragment frag;
        register_read_buffer(frag.pointer.ref_data());
        register_read_buffer(frag.size);
        co_await recv_buffers(message_header);
        co_return frag;
    }

    coro<uint128_t> recv_uint128_t(const header& message_header) {
        uint128_t num;
        register_read_buffer(num.ref_data());
        co_await recv_buffers(message_header);
        co_return num;
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    coro<T> recv_primitive(const header& message_header) {
        T val;
        register_read_buffer(val);
        co_await recv_buffers(message_header);
        co_return val;
    }

    coro<dedupe_response> recv_dedupe_response(const header& message_header) {
        dedupe_response dedupe_resp;
        register_read_buffer(dedupe_resp.effective_size);
        dedupe_resp.addr.allocate_for_serialized_data(
            message_header.size - sizeof(dedupe_resp.effective_size));
        register_read_buffer(dedupe_resp.addr.pointers);
        register_read_buffer(dedupe_resp.addr.sizes);
        co_await recv_buffers(message_header);
        co_return std::move(dedupe_resp);
    }

    coro<void> send_address(const message_type type, const address& addr) {
        register_write_buffer(addr.pointers);
        register_write_buffer(addr.sizes);
        co_await send_buffers(type);
    }

    coro<void> send_fragment(const message_type type, const fragment frag) {
        register_write_buffer(frag.pointer.get_data(), 2);
        register_write_buffer(frag.size);
        co_await send_buffers(type);
    }

    coro<void> send_uint128_t(const message_type type, const uint128_t num) {
        register_write_buffer(num.get_data(), 2);
        co_await send_buffers(type);
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    coro<void> send_primitive(const message_type type, const T val) {
        register_write_buffer(val);
        co_await send_buffers(type);
    }

    coro<void> send_dedupe_response(const dedupe_response& dedupe_resp) {
        register_write_buffer(dedupe_resp.effective_size);
        register_write_buffer(dedupe_resp.addr.pointers);
        register_write_buffer(dedupe_resp.addr.sizes);
        co_await send_buffers(SUCCESS);
    }
};

} // end namespace uh::cluster

#endif // CORE_MESSENGER_H
