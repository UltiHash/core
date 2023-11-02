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

    boost::asio::steady_timer waiter (ioc, boost::asio::steady_timer::clock_type::duration::max ());
    std::vector <std::unique_ptr <ResultType>> result (nodes.size ());
    //std::cout << "bc start" << std::endl;
    std::atomic <int> responses = 0;
    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc,
                              [&func, &nodes, &result, &waiter, &responses, id] () -> coro <message_type> {
                                    //std::cout << "before acquire " << id << std::endl;
                                    auto m = nodes[id]->acquire_messenger();
                                    //std::cout << "before co await func " << id  << std::endl;
                                    result [id] = std::make_unique <ResultType> (co_await func (std::move (m), id));

                                    auto count = responses.load();
                                    auto new_val = count + 1;
                                    while (!responses.compare_exchange_weak (count, new_val)) {
                                        count = responses.load();
                                        new_val = count + 1;
                                    }
                                    //std::cout << "after co await func " << id << " count " << new_val << std::endl;
                                    if (new_val == nodes.size()) {
                                        waiter.expires_at(boost::asio::steady_timer::time_point::min());
                                        //std::cout << "after expire " << id << std::endl;
                                    }
                                    co_return SUCCESS;
        }, boost::asio::detached);
    }

    //std::cout << "before wait" << std::endl;
    co_await waiter.async_wait(as_tuple(boost::asio::use_awaitable));
    //std::cout << "after wait" << std::endl;

    std::map <int, ResultType> result_map;
    for (int i = 0; i < result.size(); i++) {
        result_map.emplace(i, std::move (*result[i].release()));
    }

    co_return result_map;
}

template <typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <void>>;}
void broadcast_custom (boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, Func func) {

    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc, func(nodes[id].get()->acquire_messenger(), id), boost::asio::detached);
    }
}

} // end namespace uh::cluster
#endif //CORE_NETWORK_TRAITS_H
