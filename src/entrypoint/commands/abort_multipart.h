#ifndef ENTRYPOINT_HTTP_ABORT_MULTIPART_H
#define ENTRYPOINT_HTTP_ABORT_MULTIPART_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class abort_multipart : public command {
public:
    abort_multipart(multipart_state&, global_data_view&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    multipart_state& m_uploads;
    global_data_view& m_gdv;
};

} // namespace uh::cluster

#endif
