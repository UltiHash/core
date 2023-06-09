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

        /**
         * register_func is for registering cleanup functions after the signals such as SIGTERM or SIGINT is caught. This
         * function must be used for signal related cleaning up. It is important to note that the functions are executed
         * in the order that they were added.
         *
         * @param func , function to execute after signal is caught. The function in the mean time gets stored in a vector.
         */
        void register_func(std::function<void()>&& func);

        [[nodiscard]] int run() const;

    private:
        sigset_t m_sigset {};

        std::vector<std::function<void()>> m_handler_functions;

    };

// ---------------------------------------------------------------------

} // namespace uh::signal

#endif
