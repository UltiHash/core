//
// Created by massi on 12/6/23.
//

#ifndef UH_CLUSTER_UTILS_H
#define UH_CLUSTER_UTILS_H

#include <exception>
#include <boost/asio/steady_timer.hpp>
#include "common.h"
#include "network/messenger_core.h"
#include "network/client.h"

namespace uh::cluster {

    struct utils {

        template<typename Func>
        static coro <void> post_in_workers (boost::asio::thread_pool& workers, boost::asio::io_context& ioc, Func func) {
            std::exception_ptr eptr;
            boost::asio::steady_timer waiter (ioc, boost::asio::steady_timer::clock_type::duration::max ());

            auto f = [] (auto f, auto& eptr, auto& waiter) {
                try {
                    f ();
                } catch (std::exception& e) {
                    eptr = std::current_exception();
                }
                waiter.expires_at(boost::asio::steady_timer::time_point::min());
            };
            boost::asio::post (workers, std::bind (f, std::ref (func), std::ref(eptr), std::ref(waiter)));

            co_await waiter.async_wait(as_tuple(boost::asio::use_awaitable));
            if (eptr) {
                try {
                    std::rethrow_exception(eptr);
                }
                catch (std::exception& e) {
                    std::cout << "throw" << std::endl;
                    throw e;
                }
            }
        }

        template<typename Func>
        requires requires (Func& func, client::acquired_messenger& m) {{func(std::move (m))} -> std::same_as <coro <void>>;}
        static coro <void> io_thread_acquire_messenger_and_post_in_io_threads (boost::asio::thread_pool& workers,
                                                                               boost::asio::io_context& ioc,
                                                                               client& cl,
                                                                               Func func) {

            auto f = [] (auto& ioc, auto func, auto& cl) {
                boost::asio::co_spawn(ioc, std::move (func(cl.acquire_messenger())), boost::asio::use_future).get();
            };
            co_await post_in_workers (workers, ioc, std::bind (f, std::ref(ioc), std::ref (func), std::ref(cl)));

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
#endif //UH_CLUSTER_UTILS_H
