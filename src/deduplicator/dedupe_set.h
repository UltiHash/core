#ifndef UH_CLUSTER_DEDUPE_SET_H
#define UH_CLUSTER_DEDUPE_SET_H

#include "common/utils/common.h"
#include "fragment_element.h"
#include "set_log.h"
#include <common/global_data/global_data_view.h>
#include <queue>
#include <set>
#include <utility>

namespace uh::cluster {

class dedupe_set {

public:
    struct response {
        std::optional<const std::reference_wrapper<const fragment_element>> low;
        std::optional<const std::reference_wrapper<const fragment_element>>
            high;
        const std::set<fragment_element>::const_iterator hint;
    };

    dedupe_set(const std::filesystem::path& set_log_path,
               global_data_view& storage);
    void load();
    response find(std::string_view data);
    void insert(const uint128_t& pointer, const std::string_view& data,
                const std::set<fragment_element>::const_iterator& hint);

private:
    global_data_view& m_storage;
    std::set<fragment_element> m_set;
    std::shared_mutex m;
    set_log m_set_log;
};

} // end namespace uh::cluster

#endif // UH_CLUSTER_DEDUPE_SET_H
