#ifndef CORE_COMMON_NETWORK_MESSENGER_H
#define CORE_COMMON_NETWORK_MESSENGER_H

#include "common/telemetry/context.h"
#include "messenger_core.h"

namespace uh::cluster {

class messenger : public messenger_core {
public:
    using messenger_core::messenger_core;

    coro<address> recv_address(const header& message_header);

    coro<fragment> recv_fragment(const header& message_header);

    coro<uint128_t> recv_uint128_t(const header& message_header);

    template <typename T>
    requires std::is_arithmetic_v<T>
    coro<T> recv_primitive(const header& message_header) {
        T val;
        register_read_buffer(val);
        co_await recv_buffers(message_header);
        co_return val;
    }

    template <typename K, typename V>
    coro<std::map<K, V>> recv_map(context& ctx, const header& message_header) {
        unique_buffer<> buf(message_header.size);
        register_read_buffer(buf);
        co_await recv_buffers(message_header);

        std::map<K, V> res;
        zpp::bits::in{buf.span(), zpp::bits::size2b{}}(res).or_throw();
        co_return res;
    }

    coro<dedupe_response> recv_dedupe_response(const header& message_header);

    coro<void> send_ds_write(context& ctx, const ds_write_request& req);

    coro<ds_write_request> recv_ds_write(const header& message_header);

    coro<void> send_ds_read(context& ctx, const ds_read_request& req);

    coro<ds_read_request> recv_ds_read(const header& message_header);

    coro<void> send_address(context& ctx, const message_type type,
                            const address& addr);

    coro<void> send_fragment(context& ctx, const message_type type,
                             const fragment frag);

    coro<void> send_uint128_t(context& ctx, const message_type type,
                              const uint128_t num);

    template <typename T>
    requires std::is_arithmetic_v<T>
    coro<void> send_primitive(context& ctx, const message_type type,
                              const T val) {
        register_write_buffer(val);
        co_await send_buffers(ctx, type);
    }

    coro<void> send_dedupe_response(context& ctx,
                                    const dedupe_response& dedupe_resp);

    template <typename K, typename V>
    requires(std::is_arithmetic_v<K> and std::is_arithmetic_v<V>)
    coro<void> send_map(context& ctx, const message_type type,
                        const std::map<K, V>& map) {
        std::vector<char> buf;
        zpp::bits::out{buf, zpp::bits::size2b{}}(map).or_throw();
        co_await send(ctx, type, buf);
    }
};

} // end namespace uh::cluster

#endif
