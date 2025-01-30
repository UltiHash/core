#pragma once

#include "command.h"
#include "common/crypto/hash.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "common/service_interfaces/storage_interface.h"
#include "entrypoint/config.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"

namespace uh::cluster {

class put_object : public command {
public:
    put_object(boost::asio::io_context&, const entrypoint_config&, limits&,
               directory&, storage_interface&, deduplicator_interface&);

    static bool can_handle(const ep::http::request& req);

    coro<void> validate(const ep::http::request& req) override;

    coro<ep::http::response> handle(ep::http::request& req) override;

    std::string action_id() const override;

private:
    coro<dedupe_response> put_large_object(ep::http::request& req,
                                           md5& hash) const;
    coro<dedupe_response> put_small_object(ep::http::request& req,
                                           md5& hash) const;

    boost::asio::io_context& m_ioc;
    const entrypoint_config& m_config;
    directory& m_dir;
    storage_interface& m_gdv;
    limits& m_limits;
    deduplicator_interface& m_dedup;
};

} // namespace uh::cluster
