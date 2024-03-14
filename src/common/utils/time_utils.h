
#ifndef UH_CLUSTER_TIMEOUT_H
#define UH_CLUSTER_TIMEOUT_H

#include <chrono>
#include <thread>

namespace uh::cluster {

std::string get_current_ISO8601_datetime();

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
} // namespace uh::cluster

#endif // UH_CLUSTER_TIMEOUT_H
