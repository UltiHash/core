//
// Created by massi on 8/8/24.
//

#ifndef CORO_UTIL_H
#define CORO_UTIL_H

#include "common/coroutines/awaitable_promise.h"
#include "common/types/common_types.h"

namespace uh::cluster {

template <typename R, typename I>
coro<std::vector<R>> run_for_all(boost::asio::io_context& ioc,
                                 std::function<coro<R>(size_t, I)> func,
                                 const std::vector<I>& inputs) {
    std::vector<std::shared_ptr<awaitable_promise<R>>> promises;
    promises.reserve(inputs.size());

    size_t i = 0;
    for (const auto& in : inputs) {
        promises.emplace_back();
        boost::asio::co_spawn(ioc, func(i++, in),
                              use_awaitable_promise_cospawn(promises.back()));
    }

    std::vector<R> res;
    res.reserve(inputs.size());
    for (const auto& p : promises) {
        res.emplace_back(co_await p->get());
    }
    co_return res;
}

} // namespace uh::cluster

#endif // CORO_UTIL_H
