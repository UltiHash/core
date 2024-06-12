#ifndef UH_CLUSTER_TIMEOUT_H
#define UH_CLUSTER_TIMEOUT_H

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <thread>

namespace uh::cluster {
template <typename T> using coro = boost::asio::awaitable<T>;

auto wait_for_success(auto timeout, auto retry_interval, auto&& op) {
    // unit of timeout and retry_interval = second

    const auto start = std::chrono::steady_clock::now();

    std::exception_ptr eptr;
    while (
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count() < timeout) {
        try {
            return op();
        } catch (const std::exception& e) {
            eptr = std::current_exception();
        }

        std::this_thread::sleep_for(
            std::chrono::duration<double>(retry_interval));
    }

    std::rethrow_exception(eptr);
}

template <typename clock> class basic_timer;

template <typename clock>
std::ostream& operator<<(std::ostream& out, const basic_timer<clock>& t);

template <typename clock> class basic_timer {
public:
    basic_timer()
        : m_start(clock::now()) {}

    [[nodiscard]] std::chrono::duration<double> passed() const {
        return clock::now() - m_start;
    }

    void reset() { m_start = clock::now(); }

private:
    friend std::ostream& operator<< <clock>(std::ostream&,
                                            const basic_timer<clock>&);

    clock::time_point m_start;
};

template <typename clock>
std::ostream& operator<<(std::ostream& out, const basic_timer<clock>& t) {
    out << std::chrono::duration_cast<std::chrono::milliseconds>(t.passed());
    return out;
}

using timer = basic_timer<std::chrono::steady_clock>;

struct timeout {

    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::shared_ptr<boost::asio::steady_timer> m_waiter;
    explicit timeout(boost::asio::io_context& ioc)
        : m_strand(ioc.get_executor()) {}

    void start(int nsecs) {
        m_waiter = std::make_shared<boost::asio::steady_timer>(
            m_strand, std::chrono::seconds(nsecs));
        auto t = std::make_shared<timer>();

        boost::asio::co_spawn(
            m_strand, m_waiter->async_wait(boost::asio::use_awaitable),
            [t, nsecs](const std::exception_ptr& e) {

                if (auto passed = t->passed().count(); passed >= nsecs) {

                    throw std::runtime_error("timeout after " +
                                             std::to_string(passed));
                }
            });

    }

    void stop() const {

        boost::asio::post(m_strand, [waiter = m_waiter]() {
            waiter->expires_after(std::chrono::seconds(0));
        });

    }

    ~timeout() {
        stop();
    }
};
} // namespace uh::cluster

#endif // UH_CLUSTER_TIMEOUT_H
