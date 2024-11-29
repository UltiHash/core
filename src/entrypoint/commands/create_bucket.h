#ifndef ENTRYPOINT_HTTP_CREATE_BUCKET_H
#define ENTRYPOINT_HTTP_CREATE_BUCKET_H

#include "command.h"
#include "entrypoint/directory.h"

namespace uh::cluster {

class create_bucket : public command {
public:
    explicit create_bucket(directory&);

    static bool can_handle(const ep::http::request& req);

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    directory& m_dir;
};

} // namespace uh::cluster

#endif
