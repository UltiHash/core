#define BOOST_TEST_MODULE "cancellation tests"

#include <boost/asio.hpp>

#include "test_config.h"

class task_owner {
public:
    task_owner(boost::asio::io_context& ioc)
        : ioc_(ioc),
          signal_(std::make_shared<boost::asio::cancellation_signal>()) {
        boost::asio::co_spawn(ioc_, task(signal_),
                              boost::asio::bind_cancellation_slot(
                                  signal_->slot(), boost::asio::detached));
    }

    ~task_owner() { signal_->emit(boost::asio::cancellation_type::all); }

private:
    boost::asio::io_context& ioc_;
    std::shared_ptr<boost::asio::cancellation_signal> signal_;

    static boost::asio::awaitable<void>
    task(std::shared_ptr<boost::asio::cancellation_signal> signal) {
        auto state = co_await boost::asio::this_coro::cancellation_state;
        while (state.cancelled() == boost::asio::cancellation_type::none) {
            co_await boost::asio::steady_timer(
                co_await boost::asio::this_coro::executor,
                std::chrono::hours(1))
                .async_wait(boost::asio::use_awaitable);
        }
        co_return;
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
