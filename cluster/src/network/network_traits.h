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

template <typename DataPieces>
requires std::is_same_v <DataPieces, std::vector <std::vector <char>>> or std::is_same_v <DataPieces, std::vector <std::span <char>>>
std::vector <std::pair <messenger_core::header, ospan <char>>> scatter_gather (as_coroutine, boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, const message_types type, const DataPieces &data) {
    if (nodes.size() != data.size()) [[unlikely]] {
        throw std::logic_error("The count of data pieces does not match with the count of the nodes");
    }

    std::vector <client::acquired_messenger> messengers;
    messengers.reserve (data.size());
    std::vector <std::future<std::pair <messenger_core::header, ospan <char>>>> futures;
    futures.reserve(nodes.size());

    for (int i = 0; i < nodes.size(); i++) {
        messengers.emplace_back (nodes[i].get()->acquire_messenger());
        futures.emplace_back(boost::asio::co_spawn(ioc, messengers.back().get().send_recv(type, data[i]), boost::asio::use_future));
    }

    std::vector <std::pair <messenger_core::header, ospan <char>>> result;
    result.reserve (nodes.size());
    std::vector <std::future <messenger_core::header>> recv_response_futures;
    recv_response_futures.reserve(nodes.size());

    for (int i = 0; i < nodes.size(); i++) {
        result.emplace_back(futures[i].get());
    }

    return result;
}

template <typename ResultType, typename Func>
requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), int {})} -> std::same_as <coro <std::pair <messenger::header, ResultType>>>;}
std::vector <std::pair <messenger_core::header, ResultType>> broadcast_gather_custom (as_coroutine, boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, Func func) {

    std::vector <std::future<std::pair <messenger_core::header, ResultType>>> futures;
    futures.reserve(nodes.size());

    for (int id = 0; id < nodes.size(); id++) {
        futures.emplace_back(boost::asio::co_spawn(ioc, func (std::move (nodes[id].get()->acquire_messenger()), id), boost::asio::use_future));
    }

    std::vector <std::pair <messenger_core::header, ResultType>> result;
    result.reserve (nodes.size());

    for (auto& f: futures) {
        result.emplace_back(f.get());
    }

    return result;
}


std::vector <std::pair <messenger_core::header, ospan <char>>> broadcast_gather (as_coroutine, boost::asio::io_context& ioc, std::vector <std::shared_ptr <client>> &nodes, const message_types type, const std::span<char> &data) {
    std::vector <client::acquired_messenger> messengers;
    messengers.reserve (nodes.size());
    std::vector <std::future<std::pair <messenger_core::header, ospan <char>>>> futures;
    futures.reserve(nodes.size());

    for (const auto& node: nodes) {
        messengers.emplace_back (node->acquire_messenger());
        futures.emplace_back(boost::asio::co_spawn(ioc, messengers.back().get().send_recv(type, data), boost::asio::use_future));
    }

    std::vector <std::pair <messenger_core::header, ospan <char>>> result;
    result.reserve (nodes.size());
    std::vector <std::future <messenger_core::header>> recv_response_futures;
    recv_response_futures.reserve(nodes.size());

    for (int i = 0; i < nodes.size(); i++) {
        result.emplace_back(futures[i].get());
    }

    return result;
}


} // end namespace uh::cluster
#endif //CORE_NETWORK_TRAITS_H
