#define BOOST_TEST_MODULE "cancellation tests"

#include <boost/asio.hpp>

#include "test_config.h"

class task_owner {
public:
    task_owner(boost::asio::io_context& ioc)
        : m_ioc{ioc},
          m_promise{},
          m_future{m_promise.get_future()},
          m_signal{} {
        boost::asio::co_spawn(
            m_ioc,
            [this]() -> boost::asio::awaitable<void> { co_await task(); },
            boost::asio::bind_cancellation_slot(m_signal.slot(),
                                                boost::asio::detached));
    }

    ~task_owner() {
        m_signal.emit(boost::asio::cancellation_type::all);
        m_future.get();
    }

private:
    boost::asio::io_context& m_ioc;
    std::promise<void> m_promise;
    std::future<void> m_future;
    boost::asio::cancellation_signal m_signal;

    boost::asio::awaitable<void> task() {
        auto state = co_await boost::asio::this_coro::cancellation_state;
        while (state.cancelled() == boost::asio::cancellation_type::none) {
            try {
                co_await boost::asio::steady_timer(
                    co_await boost::asio::this_coro::executor,
                    std::chrono::hours(1))
                    .async_wait(boost::asio::use_awaitable);
            } catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    std::cout << "Task cancelled" << std::endl;
                    m_promise.set_value();
                    co_return;
                } else {
                    throw; // rethrow unexpected errors
                }
            }
        }
        std::cout << "Task finished" << std::endl;
        m_promise.set_value();
    }
};

BOOST_AUTO_TEST_SUITE(a_instance_which_owns_detached_tasks)

BOOST_AUTO_TEST_CASE(cancels_tasks_on_destruction) {
    boost::asio::io_context ioc;
    auto owner = std::make_optional<task_owner>(ioc);
    std::jthread thread([&ioc] { ioc.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    owner.reset();
}

BOOST_AUTO_TEST_SUITE_END()
