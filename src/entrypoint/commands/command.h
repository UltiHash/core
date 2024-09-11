#ifndef CORE_ENTRYPOINT_COMMANDS_COMMAND_H
#define CORE_ENTRYPOINT_COMMANDS_COMMAND_H

#include "entrypoint/http/request.h"
#include "entrypoint/http/response.h"

namespace uh::cluster {

class command {
public:
    virtual coro<ep::http::response> handle(ep::http::request&) = 0;
    virtual coro<void> validate(const ep::http::request& req) { co_return; }
    virtual std::string action_id() const = 0;
    virtual ~command() = default;

protected:
#ifdef DISABLE_STORAGE_REFCOUNT
    static constexpr bool m_enable_refcount = false;
#else
    static constexpr bool m_enable_refcount = true;
#endif
};

} // end namespace uh::cluster

#endif // COMMAND_H
