#pragma once

#include "common/coroutines/promise.h"
#include "common/types/common_types.h"

namespace uh::cluster {

template <typename R, typename I>
coro<std::conditional_t<std::is_void_v<R>, void, std::vector<R>>>
run_for_all(boost::asio::io_context& ioc,
            std::function<coro<R>(size_t, I)> func,
            const std::vector<I>& inputs) {
    std::vector<future<R>> futures;
    futures.reserve(inputs.size());

    size_t i = 0;
    auto context = co_await boost::asio::this_coro::context;
    for (const auto& in : inputs) {
        promise<R> p;
        futures.emplace_back(p.get_future());

        boost::asio::co_spawn(ioc, func(i++, in).continue_trace(context),
                              use_promise_cospawn(std::move(p)));
    }

    if constexpr (std::is_void_v<R>) {
        for (auto& f : futures) {
            co_await f.get();
        }
        co_return;
    } else {
        std::vector<R> res;
        res.reserve(inputs.size());
        for (auto& f : futures) {
            res.emplace_back(co_await f.get());
        }
        co_return res;
    }
}

} // namespace uh::cluster
