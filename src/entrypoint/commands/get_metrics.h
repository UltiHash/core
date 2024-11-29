#ifndef ENTRYPOINT_HTTP_GET_STATISTICS_H
#define ENTRYPOINT_HTTP_GET_STATISTICS_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class get_metrics : public command {
public:
    get_metrics(directory&, global_data_view&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
    global_data_view& m_gdv;
};

} // namespace uh::cluster

#endif
