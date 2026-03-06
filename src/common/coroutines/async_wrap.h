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
#include <boost/asio.hpp>
#include <functional>

namespace uh::cluster {

template <typename ResponseType, typename CallbackAPI, typename CompletionToken,
          typename... Args>
auto async_wrap(CallbackAPI&& api, CompletionToken&& token, Args&&... args) {
    auto init =
        [](boost::asio::completion_handler_for<void(ResponseType)> auto handler,
           CallbackAPI api, Args... args) {
            auto work = boost::asio::make_work_guard(handler);

            std::invoke(
                api, args...,
                // callback
                [handler = std::move(handler),
                 work = std::move(work)](ResponseType result) mutable {
                    auto alloc = boost::asio::get_associated_allocator(
                        handler, boost::asio::recycling_allocator<void>());

                    boost::asio::dispatch(
                        work.get_executor(),
                        boost::asio::bind_allocator(
                            alloc, [handler = std::move(handler),
                                    result = std::move(result)]() mutable {
                                std::move(handler)(std::move(result));
                            }));
                });
        };

    return boost::asio::async_initiate<CompletionToken, void(ResponseType)>(
        init, token, std::forward<CallbackAPI>(api),
        std::forward<Args>(args)...);
}

} // namespace uh::cluster
