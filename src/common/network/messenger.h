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

    coro<directory_message>
    recv_directory_message(const header& message_header) {
        unique_buffer<char> data(message_header.size);
        register_read_buffer(data);
        co_await recv_buffers(message_header);
        directory_message req;
        zpp::bits::in{data.get_span(), zpp::bits::size4b{}}(req).or_throw();
        co_return std::move(req);
    }

    coro<std::pair<bool, std::string>>
    recv_directory_get_object_chunk(const header& message_header) {

        bool has_next;
        std::string buffer(message_header.size - sizeof(bool), '\0');
        register_read_buffer(has_next);
        register_read_buffer(buffer);
        co_await recv_buffers(message_header);
        co_return std::make_pair(has_next, std::move(buffer));
    }

    coro<directory_list_buckets_message>
    recv_directory_list_buckets_message(const header& message_header) {
        unique_buffer<char> data(message_header.size);
        register_read_buffer(data);
        co_await recv_buffers(message_header);
        directory_list_buckets_message req;
        zpp::bits::in{data.get_span(), zpp::bits::size4b{}}(req).or_throw();
        co_return std::move(req);
    }

    coro<directory_list_objects_message>
    recv_directory_list_objects_message(const header& message_header) {
        unique_buffer<char> data(message_header.size);
        register_read_buffer(data);
        co_await recv_buffers(message_header);
        directory_list_objects_message req;
        zpp::bits::in{data.get_span(), zpp::bits::size4b{}}(req).or_throw();
        co_return std::move(req);
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

    coro<void> send_directory_message(const message_type type,
                                      const directory_message& dir_req) {
        std::vector<char> data;
        zpp::bits::out{data, zpp::bits::size4b{}}(dir_req).or_throw();
        register_write_buffer(data);
        co_await send_buffers(type);
    }

    coro<void> send_directory_list_buckets_message(
        const directory_list_buckets_message& dir_req) {
        std::vector<char> data;
        zpp::bits::out{data, zpp::bits::size4b{}}(dir_req).or_throw();
        register_write_buffer(data);
        co_await send_buffers(SUCCESS);
    }

    coro<void> send_directory_get_object_chunk(bool has_next,
                                               std::string buffer) {
        register_write_buffer(has_next);
        register_write_buffer(buffer);
        co_await send_buffers(SUCCESS);
    }

    coro<void> send_directory_list_objects_message(
        const directory_list_objects_message& dir_req) {
        std::vector<char> data;
        zpp::bits::out{data, zpp::bits::size4b{}}(dir_req).or_throw();
        register_write_buffer(data);
        co_await send_buffers(SUCCESS);
    }
};

} // end namespace uh::cluster

#endif // CORE_MESSENGER_H
