//
// Created by masi on 10/10/23.
//

#ifndef CORE_NETWORK_TRAITS_H
#define CORE_NETWORK_TRAITS_H

#include <vector>
#include "client.h"
#include "messenger_core.h"
#include <memory>
#include <common/awaitable_future.h>

namespace uh::cluster {

template <typename ResultType, typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <ResultType>>;}
std::map <int, ResultType> broadcast_gather_custom (std::vector <std::shared_ptr <client>> &nodes, Func func) {

    boost::asio::io_context ioc;
    std::mutex m;
    std::map <int, ResultType> results;

    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc, [&m, &func, &nodes, &results, id] () -> coro <message_type> {
                auto result = co_await func (std::move (nodes[id].get()->acquire_messenger()), id);
                std::lock_guard <std::mutex> lk (m);
                results.emplace(id, std::move (result));
                co_return SUCCESS;
            }, boost::asio::use_future);
    }

    ioc.run();
    return results;
}

template <typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <void>>;}
void broadcast_custom (as_coroutine, boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, Func func) {

    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc, func (std::move (nodes[id].get()->acquire_messenger()), id), boost::asio::detached);
    }

}

} // end namespace uh::cluster
#endif //CORE_NETWORK_TRAITS_H
