
#ifndef EC_GETTER_H
#define EC_GETTER_H
#include "common/ec/ec_scheme.h"
#include "storage/interfaces/storage_group.h"

namespace uh::cluster {

struct ec_get_handler : public service_monitor<storage_group> {
    explicit ec_get_handler(size_t data_nodes, size_t ec_nodes)
        : m_scheme(data_nodes, ec_nodes) {}

    std::shared_ptr<storage_group> get(std::size_t id) {
        return m_getter.get(m_scheme.calc_group_id(id));
    }

    std::shared_ptr<storage_group> get(const uint128_t& pointer) {
        return get(m_scheme.calc_group_id(pointer));
    }

    std::vector<std::shared_ptr<storage_group>> get_services() {
        return m_getter.get_services();
    }

private:
    void add_client(size_t id,
                    const std::shared_ptr<storage_group>& client) override {
        const auto gid = m_scheme.calc_group_id(id);
        if (!m_getter.contains(gid))
            m_getter.add_client(gid, client);
    }

    void remove_client(size_t id,
                       const std::shared_ptr<storage_group>& client) override {

        if (client->is_empty())
            m_getter.remove_client(id, client);
    }

    ec_scheme m_scheme;
    service_get_handler<storage_group> m_getter;
};
} // namespace uh::cluster

#endif // EC_GETTER_H
