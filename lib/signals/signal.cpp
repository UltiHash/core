#include <signals/signal.h>

namespace uh::signal
{

// ---------------------------------------------------------------------

signal::signal()
{
    if ( sigemptyset(&m_sigset) == -1 || sigaddset(&m_sigset, SIGINT) == -1 || sigaddset(&m_sigset, SIGTERM) == -1 )
    {
        throw std::runtime_error("sigset: Failed to initialize the signal set.");
    }

    if (pthread_sigmask(SIG_BLOCK, &m_sigset, nullptr) != 0)
    {
        throw std::runtime_error("pthread_sigmask: Failed to block the signals in the set.");
    }

    INFO << "Signal handler initialized.";
}

// ---------------------------------------------------------------------

int signal::run() const
{
    int signum = 0;

    if (sigwait(&m_sigset, &signum) != 0)
    {
        throw std::runtime_error("sigwait: Error while waiting for signals.");
    }

    DEBUG << " " << strsignal(signum) <<  "(" << signum << ") called, cleaning up ... ";

    for (const auto& cleanup_function : m_handler_functions)
    {
        cleanup_function();
    }

    return signum;
}

// ---------------------------------------------------------------------

void signal::register_func(std::function<void()>&& func)
{
    m_handler_functions.emplace_back(std::move(func));
}

// ---------------------------------------------------------------------

} //uh::signal
