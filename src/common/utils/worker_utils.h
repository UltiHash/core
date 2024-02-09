//
// Created by massi on 12/6/23.
//

#ifndef UH_CLUSTER_WORKER_UTILS_H
#define UH_CLUSTER_WORKER_UTILS_H

#include <exception>
#include <boost/asio/steady_timer.hpp>
#include "common.h"
#include "common/network/messenger_core.h"
#include "common/network/client.h"
#include "awaitable_future.h"

namespace uh::cluster {

    struct worker_utils {

        template<typename Func, typename... Args>
        requires (!std::is_void_v <std::invoke_result_t <Func, Args...>>)
        static coro <std::invoke_result_t <Func, Args...>> post_in_workers (boost::asio::thread_pool& workers, boost::asio::io_context& ioc, Func func) {
            std::exception_ptr eptr;
            awaitable_promise <std::invoke_result_t <Func, Args...>> pr (ioc);

            auto f = [] (auto& f, auto& eptr, auto& promise) {
                try {
                    promise.set(f ());
                } catch (const std::exception& e) {
                    eptr = std::current_exception();
                }
            };

            boost::asio::post (workers, std::bind (f, std::ref (func), std::ref(eptr), std::ref(pr)));
            auto r = co_await pr.get();
            if (eptr) {
                std::rethrow_exception(eptr);
            }
            co_return std::move (r);
        }

        template<typename Func, typename... Args>
        requires (std::is_void_v <std::invoke_result_t <Func, Args...>>)
        static coro <void> post_in_workers (boost::asio::thread_pool& workers, boost::asio::io_context& ioc, Func func) {
            std::exception_ptr eptr;
            awaitable_promise <void> pr (ioc);

            auto f = [] (auto& f, auto& eptr, auto& promise) {
                try {
                    f ();
                } catch (const std::exception& e) {
                    eptr = std::current_exception();
                }
                promise.set();
            };

            boost::asio::post (workers, std::bind (f, std::ref (func), std::ref(eptr), std::ref(pr)));
            co_await pr.get();
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        }

        template<typename Func>
        requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m))} -> std::same_as <coro <void>>;}
        static coro <void> io_thread_acquire_messenger_and_post_in_io_threads (boost::asio::thread_pool& workers,
                                                                               boost::asio::io_context& ioc,
                                                                               std::shared_ptr<client> cl,
                                                                               Func func) {

            auto f = [] (auto& cl) -> client::acquired_messenger {
                return cl->acquire_messenger ();
            };
            auto m = co_await post_in_workers (workers, ioc, std::bind (f, std::ref(cl)));
            co_await func (std::move (m));
        }

        template<typename Func>
        requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), long {})} -> std::same_as <coro <void>>;}
        static void broadcast_from_worker_in_io_threads (const std::vector<std::shared_ptr<client>>& nodes, boost::asio::io_context& ioc, Func func) {
            std::vector <std::future <void>> futures;
            futures.reserve(nodes.size());

            long i = 0;
            for (auto& n: nodes) {
                auto m = n->acquire_messenger();
                futures.emplace_back (boost::asio::co_spawn(ioc,
                                                            func (std::move (m), i++),
                                                            boost::asio::use_future));
            }

            for (auto& f: futures) {
                f.get();
            }
        }

        template <typename Func>
        requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m), long {})} -> std::same_as <coro <void>>;}
        static coro <void> broadcast_from_io_thread_in_io_threads (const std::vector<std::shared_ptr<client>>& nodes,
                                                               boost::asio::io_context& ioc,
                                                               boost::asio::thread_pool& workers,
                                                               Func func) {
            auto f = [] (const auto& nodes, auto& ioc, auto func) {broadcast_from_worker_in_io_threads (nodes, ioc, std::move (func));};
            co_await post_in_workers (workers, ioc, std::bind (f, std::cref (nodes), std::ref (ioc), std::move (func)));
        }

    };

} // end namespace uh::cluster
#endif //UH_CLUSTER_WORKER_UTILS_H
