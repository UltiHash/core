#ifndef UH_CLUSTER_LIST_OBJECTSV2_H
#define UH_CLUSTER_LIST_OBJECTSV2_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

struct list_objects_v2 : public command {

    explicit list_objects_v2(directory& dir);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_LIST_OBJECTSV2_H
