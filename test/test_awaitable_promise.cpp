#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "awaitable_promise tests"
#endif

#include <boost/test/unit_test.hpp>
#include "common/utils/awaitable_promise.h"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(basic_promise) {

    boost::asio::io_context ioc;

    boost::asio::co_spawn(ioc, [&ioc] () -> coro <void> {
        awaitable_promise <int> pr (ioc);
        ioc.post([&pr] () {pr.set(1);});
        BOOST_TEST((co_await pr.get()) == 1);
        },
        [] (const std::exception_ptr& e){if (e) std::rethrow_exception(e);});

    ioc.run();
}

BOOST_AUTO_TEST_CASE(promise_exception) {

    boost::asio::io_context ioc;

    boost::asio::co_spawn(ioc, [&ioc] () -> coro <void> {
                              awaitable_promise <int> pr (ioc);
                              ioc.post([&pr] () {try {
                                  throw std::exception {};
                              }
                              catch (const std::exception& e) {
                                  pr.set_exception(std::current_exception());
                              }
                              });
                              BOOST_CHECK_THROW ((co_await pr.get()), std::exception);
                          },
                          [] (const std::exception_ptr& e){if (e) std::rethrow_exception(e);});

    ioc.run();
}

BOOST_AUTO_TEST_CASE(stress_test) {

    boost::asio::io_context ioc;

    int thread_count = 1000;
    int task_count = 1000000;

    std::vector <std::thread> threads;
    threads.reserve(thread_count);

    std::atomic<int> failures = 0;

    for (int i = 0; i < task_count; i++) {
        boost::asio::co_spawn(ioc, [&ioc, i, &failures] () -> coro <void> {
                                  awaitable_promise <int> pr (ioc);
                                  ioc.post([&pr, i] () {pr.set(int (i));});
                                  if ((co_await pr.get()) != i) {
                                      failures ++;
                                  }
                              },
                              [] (const std::exception_ptr& e){if (e) std::rethrow_exception(e);});
    }

    for (int i = 0; i < thread_count; i++) {
        threads.emplace_back([&ioc] () {ioc.run();});
    }

    for (auto& t: threads) {
        t.join();
    }

    BOOST_TEST(failures == 0);

}

} // namespace uh::cluster
