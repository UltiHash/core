#ifndef ENTRYPOINT_HTTP_LIST_MULTIPART_H
#define ENTRYPOINT_HTTP_LIST_MULTIPART_H

#include "command.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class list_multipart : public command {
public:
    explicit list_multipart(multipart_state&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    multipart_state& m_uploads;
};

} // namespace uh::cluster

#endif
