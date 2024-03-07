#ifndef UH_CLUSTER_FRAGMENT_SET_H
#define UH_CLUSTER_FRAGMENT_SET_H

#include "common/utils/common.h"
#include "fragment_set_element.h"
#include "fragment_set_log.h"
#include <common/global_data/global_data_view.h>
#include <queue>
#include <set>
#include <utility>

namespace uh::cluster {

class fragment_set {

public:
    struct response {
        std::optional<const std::reference_wrapper<const fragment_set_element>>
            low;
        std::optional<const std::reference_wrapper<const fragment_set_element>>
            high;
        const std::set<fragment_set_element>::const_iterator hint;
    };

    fragment_set(const std::filesystem::path& set_log_path,
                 global_data_view& storage);
    response find(std::string_view data);
    void insert(const uint128_t& pointer, const std::string_view& data,
                const std::set<fragment_set_element>::const_iterator& hint);

private:
    global_data_view& m_storage;
    std::set<fragment_set_element> m_set;
    std::shared_mutex m_mutex;
    fragment_set_log m_set_log;
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_SET_H
