/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "common/coroutines/promise.h"
#include <exception>
#include <memory>

namespace uh::cluster {

class worker_pool {

public:
    worker_pool(size_t worker_count)
        : m_threads(worker_count) {}

    template <typename Func>
    requires(!std::is_void_v<std::invoke_result_t<Func>>)
    coro<std::invoke_result_t<Func>> post_in_workers(Func func) {
        promise<std::invoke_result_t<Func>> p;
        auto fut = p.get_future();

        auto context = co_await boost::asio::this_coro::context;
        auto f = [context](auto& f, auto&& promise) mutable {
            THREAD_LOCAL_CONTEXT = context;

            if (boost::asio::trace_span::enable &&
                !boost::asio::trace_span::check_context(context)) {
                LOG_ERROR() << "[post_in_workers] The context to be "
                               "encoded is invalid";
            }

            try {
                promise.set_value(f());
            } catch (const std::exception&) {
                promise.set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads,
                          std::bind(f, std::ref(func), std::move(p)));

        co_return co_await fut.get();
    }

    template <typename Func>
    requires(std::is_void_v<std::invoke_result_t<Func>>)
    coro<void> post_in_workers(Func func) {
        promise<void> p;
        auto fut = p.get_future();

        auto context = co_await boost::asio::this_coro::context;

        auto f = [context](auto& f, auto&& promise) mutable {
            try {
                THREAD_LOCAL_CONTEXT = context;

                if (boost::asio::trace_span::enable &&
                    !boost::asio::trace_span::check_context(context)) {
                    LOG_ERROR() << "[post_in_workers] The context to be "
                                   "encoded is invalid";
                }

                f();
                promise.set_value();
            } catch (const std::exception&) {
                promise.set_exception(std::current_exception());
            }
        };

        boost::asio::post(m_threads,
                          std::bind(f, std::ref(func), std::move(p)));

        co_await fut.get();
    }

    ~worker_pool() {
        m_threads.join();
        m_threads.stop();
    }

private:
    boost::asio::thread_pool m_threads;
};

} // end namespace uh::cluster
