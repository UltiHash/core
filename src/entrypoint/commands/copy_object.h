#ifndef ENTRYPOINT_HTTP_COPY_OBJECT_H
#define ENTRYPOINT_HTTP_COPY_OBJECT_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class copy_object : public command {
public:
    copy_object(directory&, global_data_view&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

    std::string action_id() const override;

private:
    coro<void> copy_internal(http_request& req, std::string& src_bucket,
                             std::string& src_key);

    directory& m_directory;
    global_data_view& m_gdv;
};

} // namespace uh::cluster

#endif
