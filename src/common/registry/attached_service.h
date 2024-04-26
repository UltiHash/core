
#ifndef UH_CLUSTER_ATTACHED_SERVICE_H
#define UH_CLUSTER_ATTACHED_SERVICE_H

#include "config/configuration.h"
namespace uh::cluster {

template <typename service> class attached_service {

public:
    template <typename Conf>
    attached_service(const service_config& sc,
                     const std::optional<Conf>& conf) {
        if (conf) {
            m_attached_service.emplace(sc, *conf);
            m_local_service_thread =
                std::thread([this] { m_attached_service->run(); });
        }
    }

    auto get_local_service_interface() {
        return (m_attached_service) ? m_attached_service->get_local_interface()
                                    : nullptr;
    }

    ~attached_service() {
        if (m_attached_service) {
            m_attached_service->stop();
            m_local_service_thread.join();
        }
    }

private:
    std::optional<service> m_attached_service;
    std::thread m_local_service_thread;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_ATTACHED_SERVICE_H
