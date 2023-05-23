#ifndef SIGNALS_SIGNAL_H
#define SIGNALS_SIGNAL_H

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

    private:
        static void handler(int signal_number);

        static std::vector<std::function<void()>> m_handler_functions;

    };

// ---------------------------------------------------------------------

} // namespace uh::signal

#endif
