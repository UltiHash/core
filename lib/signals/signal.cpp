#include <signals/signal.h>
#include <util/exception.h>

namespace uh::signal
{

// ---------------------------------------------------------------------

signal::signal()
{
    if ( sigemptyset(&m_sigset) == -1 || sigaddset(&m_sigset, SIGINT) == -1 || sigaddset(&m_sigset, SIGTERM) == -1 )
    {
        THROW_FROM_ERRNO();
    }

    if (pthread_sigmask(SIG_BLOCK, &m_sigset, nullptr) != 0)
    {
        THROW_FROM_ERRNO();
    }

    INFO << "Signal handler initialized.";
}

// ---------------------------------------------------------------------

int signal::run() const
{
    int signum = 0;

    if (sigwait(&m_sigset, &signum) != 0)
    {
        THROW_FROM_ERRNO();
    }

    DEBUG << " " << strsignal(signum) <<  "(" << signum << ") called, cleaning up ";

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
