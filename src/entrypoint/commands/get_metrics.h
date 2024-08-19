#ifndef ENTRYPOINT_HTTP_GET_STATISTICS_H
#define ENTRYPOINT_HTTP_GET_STATISTICS_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

struct get_metrics : public command {

    get_metrics(directory&, global_data_view&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    directory& m_directory;
    global_data_view& m_gdv;
};

} // namespace uh::cluster

#endif
