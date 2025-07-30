#define BOOST_TEST_MODULE "task tests"

#include <boost/asio.hpp>
#include <common/coroutines/coro_util.h>
#include <common/types/common_types.h>

#include "test_config.h"

namespace uh::cluster {

class task_owner {
public:
    task_owner(boost::asio::io_context& ioc)
        : m_task{"task", ioc} {
        m_task.spawn(job().start_trace());
    }

    ~task_owner() {}

private:
    coro_task m_task;

    coro<void> job() {
        auto state = co_await boost::asio::this_coro::cancellation_state;
        while (state.cancelled() == boost::asio::cancellation_type::none) {
            auto executor = co_await boost::asio::this_coro::executor;
            auto timer =
                boost::asio::steady_timer(executor, std::chrono::hours(1));
            co_await timer.async_wait(boost::asio::use_awaitable);
        }
        std::cout << "Job finished" << std::endl;
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

} // namespace uh::cluster
