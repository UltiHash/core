#ifndef UH_CLUSTER_LIST_OBJECTSV2_H
#define UH_CLUSTER_LIST_OBJECTSV2_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class list_objects_v2 : public command {
public:
    explicit list_objects_v2(directory& dir);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LIST_OBJECTSV2_H
