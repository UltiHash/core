//
// Created by massi on 8/14/24.
//

#ifndef EC_LOAD_BALANCER_H
#define EC_LOAD_BALANCER_H
#include "roundrobin_load_balancer.h"
#include "service_monitor.h"
#include "storage/interfaces/storage_system.h"

namespace uh::cluster {

class ec_load_balancer : public service_monitor<storage_system> {

    void add_client(size_t id,
                    const std::shared_ptr<storage_system>& cl) override {
        if (cl->is_healthy()) {
            m_load_balancer.add_client(id, cl);
        }
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_system>& cl) override {
        m_load_balancer.remove_client(id, cl);
    }

    roundrobin_load_balancer<storage_system> m_load_balancer;

public:
    [[nodiscard]] std::shared_ptr<storage_system> get() const {
        return m_load_balancer.get();
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_load_balancer.empty();
    }
};

} // namespace uh::cluster

#endif // EC_LOAD_BALANCER_H
