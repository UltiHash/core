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

    std::mutex mut;
    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc,
                              [&func, &nodes, &result_map, &waiter, &mut, id] () -> coro <message_type> {
                                    auto m = nodes[id]->acquire_messenger();

                                    auto res = co_await func (std::move (m), id);
                                    std::lock_guard lk (mut);
                                    result_map.emplace(id, std::move (res));
                                    if (result_map.size() == nodes.size()) {
                                        waiter.expires_at(boost::asio::steady_timer::time_point::min());
                                    }
                                    co_return SUCCESS;
        }, boost::asio::detached);
    }

    co_await waiter.async_wait(as_tuple(boost::asio::use_awaitable));

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

} // end namespace uh::cluster
#endif //CORE_NETWORK_TRAITS_H
