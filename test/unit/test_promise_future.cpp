#define BOOST_TEST_MODULE "awaitable_future tests"

#include <common/coroutines/awaitable_future.h>

#include <boost/test/unit_test.hpp>
#include <util/coroutine.h>

using namespace boost::asio;

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

traced_awaitable<int> cb(awaitable_future<int>& af) { co_return co_await af; };

BOOST_FIXTURE_TEST_CASE(future_get, coro_fixture) {
    {
        awaitable_future<int> af;

        auto f = co_spawn(m_ctx, cb(af), use_future);

        af.set(4711);
        BOOST_CHECK(f.get() == 4711);
    }
    {
        awaitable_future<int> af;

        af.set(4711);
        auto f = co_spawn(m_ctx, cb(af), use_future);

        BOOST_CHECK(f.get() == 4711);
    }
}

// BOOST_AUTO_TEST_CASE(basic) {
//     {
//         std::future<int> f;
//         BOOST_CHECK(!f.valid());
//     }
//
//     {
//         std::promise<int> p;
//         std::future<int> f = p.get_future();
//         BOOST_CHECK(f.valid());
//     }
// }
//
// BOOST_FIXTURE_TEST_CASE(pass_exception, coro_fixture) {
//     uh::cluster::promise<int> p;
//
//     auto f = [&]() -> coro<int> {
//         auto f = p.get_future();
//         co_return co_await f.get();
//     };
//     std::future<int> res = spawn(f);
//
//     try {
//         throw 1;
//     } catch (...) {
//         p.set_exception(std::current_exception());
//     }
//
//     BOOST_CHECK_THROW(res.get(), int);
// }
//
// BOOST_FIXTURE_TEST_CASE(errors, coro_fixture) {
//     {
//         std::promise<int> p;
//         std::promise<int> p2 = std::move(p);
//         BOOST_CHECK_THROW(p.get_future(), std::future_error);
//     }
//     {
//         std::promise<int> p;
//         auto f = p.get_future();
//         BOOST_CHECK_THROW(p.get_future(), std::future_error);
//     }
//     {
//         std::promise<int> p;
//         std::promise<int> p2 = std::move(p);
//         BOOST_CHECK_THROW(p.set_value(1), std::future_error);
//     }
//     {
//         std::promise<int> p;
//         p.set_value(1);
//         BOOST_CHECK_THROW(p.set_value(2), std::future_error);
//     }
//     {
//         std::promise<int> p;
//         try {
//             throw 1;
//         } catch (...) {
//             p.set_exception(std::current_exception());
//         }
//         BOOST_CHECK_THROW(p.set_value(1), std::future_error);
//     }
//     {
//         std::promise<int> p;
//         p.set_value(1);
//         try {
//             throw 1;
//         } catch (...) {
//             BOOST_CHECK_THROW(p.set_exception(std::current_exception()),
//                               std::future_error);
//         }
//     }
// }

} // namespace uh::cluster
