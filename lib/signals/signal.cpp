#include <signals/signal.h>
#include <csignal>

namespace uh::signal
{

// ---------------------------------------------------------------------

signal::signal()
{
    std::signal(SIGINT, signal::handler);
    std::signal(SIGTERM, signal::handler);
}

// ---------------------------------------------------------------------

std::vector<std::function<void()>> signal::m_handler_functions;

// ---------------------------------------------------------------------

void signal::handler(int signal_number)
{
    INFO << "signal handler called: " << signal_number;

    for (const auto& cleanup_function : m_handler_functions)
    {
        cleanup_function();
    }

    std::exit(0);
}

// ---------------------------------------------------------------------

void signal::register_func(std::function<void()>&& func)
{
    m_handler_functions.emplace_back(std::move(func));
}

// ---------------------------------------------------------------------

} //uh::signal
