
#ifndef UH_CLUSTER_CORO_UTILS_H
#define UH_CLUSTER_CORO_UTILS_H

#include "common/network/client.h"

namespace uh::cluster {

template <typename func, typename result = std::invoke_result<
                             func, acquired_messenger<coro_client>,
                             std::size_t>::type::value_type>
requires(std::is_same_v<result, void>)
coro<void> broadcast(boost::asio::io_context& ioc, func f, const auto& services) {
    if (services.empty()) {
        throw std::runtime_error("no services available");
    }

    std::vector<std::shared_ptr<awaitable_promise<void>>> results(
        services.size());

    std::size_t index = 0;
    for (const auto& c : services) {
        auto promise = std::make_shared<awaitable_promise<void>>(ioc);

        auto messenger = co_await c->acquire_messenger();
        boost::asio::co_spawn(ioc, f(std::move(messenger), index),
                              use_awaitable_promise_cospawn(promise));

        results[index] = promise;
        ++index;
    }

    for (auto& p : results) {
        co_await p->get();
    }
}

template <typename func, typename result = std::invoke_result<
                             func, acquired_messenger<coro_client>,
                             std::size_t>::type::value_type>
requires (!std::is_same_v<result, void>)
coro<std::vector<result>> broadcast(boost::asio::io_context& ioc, func f, const auto& services) {
    if (services.empty()) {
        throw std::runtime_error("no services available");
    }

    std::vector<std::shared_ptr<awaitable_promise<result>>> results(
        services.size());

    std::size_t index = 0;
    for (const auto& c : services) {
        auto promise =
            std::make_shared<awaitable_promise<coro<result>>>(ioc);

        auto messenger = co_await c->acquire_messenger();
        boost::asio::co_spawn(ioc, f(messenger, index),
                              use_awaitable_promise_cospawn(promise));

        results[index] = promise;
        ++index;
    }

    std::vector<result> rv;
    for (auto& p : results) {
        rv.push_back(co_await p->get());
    }

    co_return rv;
}
}
#endif // UH_CLUSTER_CORO_UTILS_H
