//
// Created by masi on 10/10/23.
//

#ifndef CORE_NETWORK_TRAITS_H
#define CORE_NETWORK_TRAITS_H

#include <vector>
#include "client.h"
#include "messenger_core.h"
#include <memory>

namespace uh::cluster {

template <typename ResultType, typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <ResultType>>;}
coro <std::map <int, ResultType>> broadcast_gather_custom (boost::asio::io_context& ioc, const std::vector <std::shared_ptr <client>> &nodes, Func func) {

    if (nodes.empty()) [[unlikely]] {
        throw std::length_error ("Could not broadcast to empty nodes");
    }

    boost::asio::steady_timer waiter (ioc, boost::asio::steady_timer::clock_type::duration::max ());

    std::map <int, ResultType> result_map;

    //std::cout << "bc start" << std::endl;

    std::mutex mut;
    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc,
                              [&func, &nodes, &result_map, &waiter, &mut, id] () -> coro <message_type> {
                                    //std::cout << "before acquire " << id << std::endl;
                                    auto m = nodes[id]->acquire_messenger();
                                    //std::cout << "before co await func " << id  << std::endl;

                                    auto res = co_await func (std::move (m), id);
                                    std::lock_guard lk (mut);
                                    result_map.emplace(id, std::move (res));
                                    //std::cout << "after co await func " << id << " count " << new_val << std::endl;
                                    if (result_map.size() == nodes.size()) {
                                        waiter.expires_at(boost::asio::steady_timer::time_point::min());
                                        //std::cout << "after expire " << id << std::endl;
                                    }
                                    co_return SUCCESS;
        }, boost::asio::detached);
    }

    //std::cout << "before wait" << std::endl;
    co_await waiter.async_wait(as_tuple(boost::asio::use_awaitable));
    //std::cout << "after wait" << std::endl;

    co_return result_map;
}

template <typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <void>>;}
void broadcast_custom (boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, Func func) {

    if (nodes.empty()) [[unlikely]] {
        throw std::length_error ("Could not broadcast to empty nodes");
    }

    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc, func(nodes[id].get()->acquire_messenger(), id), boost::asio::detached);
    }
}

template <typename DataType, typename Func>
requires requires (Func& func, messenger& m) {{func(m)} -> std::same_as <coro <void>>;}
coro <void> send_in_chunks (messenger& m, const std::forward_list <DataType>& chunks, message_type type, size_t max_chunk_size, Func&& response_op) {

    for (const auto& c:chunks) {
        if (c.size () > max_chunk_size) [[unlikely]] {
            throw std::overflow_error ("Chunks size is not allowed to be larger than the max chunk size");
        }
        if (m.get_buffered_write_size() > max_chunk_size) {
            co_await m.send_buffers(type);
            co_await response_op;
        }
        m.register_write_buffer(c);
    }
    if (m.get_buffered_write_size() > 0) {
        co_await m.send_buffers(type);
        co_await response_op;
    }

}

} // end namespace uh::cluster
#endif //CORE_NETWORK_TRAITS_H
