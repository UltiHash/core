//
// Created by massi on 2/5/24.
//

#ifndef UH_CLUSTER_SIGNAL_HANDLER_H
#define UH_CLUSTER_SIGNAL_HANDLER_H

#include <csignal>
#include <vector>
#include <functional>

namespace uh::cluster {
class signal_handler {

    boost::asio::io_context m_ioc;
    boost::asio::signal_set m_signals;
    std::thread m_signal_handler_thread;
    std::vector <std::function <void (void)>> m_callbacks;

public:

    signal_handler (): m_ioc (1),
                    m_signals (m_ioc, SIGINT, SIGTERM) {

        m_signals.async_wait([this] (const boost::system::error_code& error, int signal) {
            signal_handler_fn (error, signal);});
        m_signal_handler_thread = std::thread ([this] () {m_ioc.run();});
    }

    void add_callback (const std::function <void (void)>& fn) {
        m_callbacks.emplace_back(fn);
    }

     void signal_handler_fn (const boost::system::error_code& error, int signal) {
        for (auto& fn: m_callbacks) {
            fn ();
        }
    }

    ~signal_handler() {
        m_signal_handler_thread.join();
     }

};

}
#endif //UH_CLUSTER_SIGNAL_HANDLER_H
