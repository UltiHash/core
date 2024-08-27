#ifndef UH_CLUSTER_TIMEOUT_H
#define UH_CLUSTER_TIMEOUT_H

#include <chrono>
#include <thread>

namespace uh::cluster {
template <typename T> using coro = boost::asio::awaitable<T>;

template <typename T, typename R>
auto wait_for_success(std::chrono::duration<T, R> timeout,
                      std::chrono::duration<T, R> retry_interval, auto&& op) {

    const auto start = std::chrono::steady_clock::now();

    std::exception_ptr eptr;
    do {
        try {
            return op();
        } catch (const std::exception& e) {
            eptr = std::current_exception();
        }

        std::this_thread::sleep_for(retry_interval);
    } while ((std::chrono::steady_clock::now() - start) < timeout);

    std::rethrow_exception(eptr);
}

template <typename T, typename R>
auto wait_for_true(std::chrono::duration<T, R> timeout,
                   std::chrono::duration<T, R> retry_interval, auto&& op) {

    const auto start = std::chrono::steady_clock::now();

    do {
        if (op())
            return;
        std::this_thread::sleep_for(retry_interval);
    } while ((std::chrono::steady_clock::now() - start) < timeout);

    throw std::runtime_error("waiting timeout");
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

    typename clock::time_point m_start;
};

template <typename clock>
std::ostream& operator<<(std::ostream& out, const basic_timer<clock>& t) {
    out << std::chrono::duration_cast<std::chrono::milliseconds>(t.passed());
    return out;
}

using timer = basic_timer<std::chrono::steady_clock>;

} // namespace uh::cluster

#endif // UH_CLUSTER_TIMEOUT_H
