#ifndef EC_LOAD_BALANCER_H
#define EC_LOAD_BALANCER_H
#include "roundrobin_load_balancer.h"
#include "service_monitor.h"
#include "storage/interfaces/storage_group.h"

namespace uh::cluster {

struct ec_load_balancer : public service_monitor<storage_group> {

    std::shared_ptr<storage_group> get() { return m_load_balancer.get(); }

    [[nodiscard]] bool empty() const noexcept {
        return m_load_balancer.empty();
    }

private:
    void add_client(size_t id,
                    const std::shared_ptr<storage_group>& cl) override {
        if (cl->is_healthy()) {
            m_load_balancer.add_client(id, cl);
        }
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_group>& cl) override {
        m_load_balancer.remove_client(id, cl);
    }

    roundrobin_load_balancer<storage_group> m_load_balancer;
};

} // namespace uh::cluster

#endif // EC_LOAD_BALANCER_H
