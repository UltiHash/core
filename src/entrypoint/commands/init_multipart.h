#ifndef UH_CLUSTER_INIT_MULTIPART_H
#define UH_CLUSTER_INIT_MULTIPART_H

#include "command.h"
#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class init_multipart : public command {
public:
    explicit init_multipart(directory&, multipart_state&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    directory& m_directory;
    multipart_state& m_uploads;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_INIT_MULTIPART_H
