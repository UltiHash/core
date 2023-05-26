#include <signals/signal.h>

namespace uh::signal
{

// ---------------------------------------------------------------------

signal::signal()
{
    sigemptyset(&m_sigset);
    sigaddset(&m_sigset, SIGINT);
    sigaddset(&m_sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &m_sigset, nullptr);
}

// ---------------------------------------------------------------------

int signal::run() const
{
    int signum = 0;
    sigwait(&m_sigset, &signum);

    INFO << "signal handler called: " << strsignal(signum) <<  "(" << signum << "), cleaning up ... ";

    for (const auto& cleanup_function : m_handler_functions)
    {
        cleanup_function();
    }

    INFO << "cleanup finished ... ";

    return signum;
}

// ---------------------------------------------------------------------

void signal::register_func(std::function<void()>&& func)
{
    m_handler_functions.emplace_back(std::move(func));
}

// ---------------------------------------------------------------------

} //uh::signal
