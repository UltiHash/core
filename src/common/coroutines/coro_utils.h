
#ifndef UH_CLUSTER_CORO_UTILS_H
#define UH_CLUSTER_CORO_UTILS_H

#include "awaitable_promise.h"

namespace uh::cluster {

template <typename service_interface, typename func,
          typename result = std::invoke_result_t<
              func, std::shared_ptr<service_interface>, std::size_t>>
requires(std::is_same_v<result, coro<void>>)
coro<void>
broadcast(boost::asio::io_context& ioc, func f,
          const std::vector<std::shared_ptr<service_interface>>& services) {
    if (services.empty()) {
        throw std::runtime_error("no services available");
    }

    std::vector<std::shared_ptr<awaitable_promise<void>>> results(
        services.size());

    std::size_t index = 0;
    for (const auto& c : services) {
        auto promise = std::make_shared<awaitable_promise<void>>(ioc);

        boost::asio::co_spawn(ioc, f(c, index),
                              use_awaitable_promise_cospawn(promise));

        results[index] = promise;
        ++index;
    }

    for (auto& p : results) {
        co_await p->get();
    }
}

} // namespace uh::cluster
#endif // UH_CLUSTER_CORO_UTILS_H
