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

std::vector <std::pair <messenger_core::header, ospan <char>>> scatter_gather (as_coroutine, std::shared_ptr <boost::asio::io_context> ioc, std::vector <client> &nodes, const message_types type, const std::vector <std::span<char>> &data) {
    if (nodes.size() != data.size()) [[unlikely]] {
        throw std::logic_error("The count of data pieces does not match with the count of the nodes");
    }

    std::vector <client::acquired_messenger> messengers;
    messengers.reserve (data.size());
    std::vector <std::future<std::pair <messenger_core::header, ospan <char>>>> futures;
    for (int i = 0; i < nodes.size(); i++) {
        messengers.emplace_back (nodes[i].acquire_messenger());
        futures.emplace_back(boost::asio::co_spawn(*ioc, messengers.back().get().send_recv(type, data[i]), boost::asio::use_future));
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
