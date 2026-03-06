/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "messenger_core.h"

namespace uh::cluster {

struct write_request_store {
    allocation_t allocation;
    std::vector<std::span<const char>> buffers;
    std::vector<refcount_t> refcounts;
    unique_buffer<> backing_buffer;
};

struct write_request_view {
    allocation_t allocation;
    std::vector<std::span<const char>> buffers;
    std::vector<refcount_t> refcounts;
};

class messenger : public messenger_core {
public:
    using messenger_core::messenger_core;

    coro<storage_address> recv_address(const header& message_header);

    coro<fragment> recv_fragment(const header& message_header);

    coro<allocation_t> recv_allocation(const header& message_header);

    coro<std::vector<refcount_t>> recv_refcounts(const header& message_header);

    template <typename T>
    requires std::is_arithmetic_v<T>
    coro<T> recv_primitive(const header& message_header) {
        T val;
        register_read_buffer(val);
        co_await recv_buffers(message_header);
        co_return val;
    }

    template <typename K, typename V>
    coro<std::map<K, V>> recv_map(const header& message_header) {
        unique_buffer<> buf(message_header.size);
        register_read_buffer(buf);
        co_await recv_buffers(message_header);

        std::map<K, V> res;
        zpp::bits::in{buf.span(), zpp::bits::size2b{}}(res).or_throw();
        co_return res;
    }

    coro<dedupe_response> recv_dedupe_response(const header& message_header);

    coro<void> send_write(const write_request_view& req);

    coro<write_request_store> recv_write(const header& message_header);

    coro<void> send_address(const message_type type,
                            const storage_address& addr);

    coro<void> send_fragment(const message_type type, const fragment frag);

    coro<void> send_allocation(const message_type type,
                               const allocation_t& allocation);

    coro<void> send_refcounts(const message_type type,
                              const std::vector<refcount_t>& refcounts);

    template <typename T>
    requires std::is_arithmetic_v<T>
    coro<void> send_primitive(const message_type type, const T val) {
        register_write_buffer(val);
        co_await send_buffers(type);
    }

    coro<void> send_dedupe_response(const dedupe_response& dedupe_resp);

    template <typename K, typename V>
    requires(std::is_arithmetic_v<K> and std::is_arithmetic_v<V>)
    coro<void> send_map(const message_type type, const std::map<K, V>& map) {
        std::vector<char> buf;
        zpp::bits::out{buf, zpp::bits::size2b{}}(map).or_throw();
        co_await send(type, buf);
    }
};

} // end namespace uh::cluster
