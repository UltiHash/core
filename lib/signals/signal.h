#ifndef SIGNALS_SIGNAL_H
#define SIGNALS_SIGNAL_H

#include <csignal>
#include <future>
#include <logging/logging_boost.h>

namespace uh::signal
{

// ---------------------------------------------------------------------

    class signal
    {
    public:
        signal();
        ~signal() = default;

        void register_func(std::function<void()>&& func);
        std::future<int> run();

    private:
        sigset_t m_sigset {};
        std::vector<std::function<void()>> m_handler_functions;

    };

// ---------------------------------------------------------------------

} // namespace uh::signal

#endif
