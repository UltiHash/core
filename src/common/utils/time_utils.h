#ifndef UH_CLUSTER_TIMEOUT_H
#define UH_CLUSTER_TIMEOUT_H

#include <chrono>
#include <thread>

namespace uh::cluster {

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

    std::chrono::duration<double> passed() const {
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

} // namespace uh::cluster

#endif // UH_CLUSTER_TIMEOUT_H
