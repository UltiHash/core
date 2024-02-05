//
// Created by massi on 2/5/24.
//

#ifndef UH_CLUSTER_SIGNAL_HANDLER_H
#define UH_CLUSTER_SIGNAL_HANDLER_H

#include <csignal>
#include <vector>
#include <functional>

namespace {
    volatile std::sig_atomic_t gSignalStatus;
}

void signal_handler_fn (int signal) {

}
namespace uh::cluster {
class signal_handler {

    inline static std::vector <std::function <void (void)>> m_callbacks;

public:

    static void init_signals () {
        std::signal(SIGINT, signal_handler_fn);
        std::signal(SIGTERM, signal_handler_fn);
    }

    static void add_callback (const std::function <void (void)>& fn) {
        m_callbacks.emplace_back(fn);
    }

    static void signal_handler_fn (int signal) {
        for (auto& fn: m_callbacks) {
            fn ();
        }
        sleep(2);
    }

};

}
#endif //UH_CLUSTER_SIGNAL_HANDLER_H
