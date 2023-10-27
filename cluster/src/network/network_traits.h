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
std::vector <ResultType> broadcast_gather_custom (boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, Func func) {

    std::vector <std::future <ResultType>> futures;
    std::vector <ResultType> results;

    futures.reserve (nodes.size());
    results.reserve (nodes.size());

    for (int id = 0; id < nodes.size(); id++) {
        futures.emplace_back (boost::asio::co_spawn(ioc, func (nodes[id].get()->acquire_messenger(), id), boost::asio::use_future));
    }

    std::thread tr ([&results, &futures] {
        for (auto& f: futures) {
            results.emplace_back (std::move (f.get()));
        }});
    tr.join();

    return results;
}

template <typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <void>>;}
void broadcast_custom (boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, Func func) {

    for (int id = 0; id < nodes.size(); id++) {
        boost::asio::co_spawn(ioc, func (nodes[id].get()->acquire_messenger(), id), boost::asio::detached);
    }

}

} // end namespace uh::cluster
#endif //CORE_NETWORK_TRAITS_H
