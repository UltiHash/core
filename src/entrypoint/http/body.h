#ifndef CORE_ENTRYPOINT_HTTP_BODY_H
#define CORE_ENTRYPOINT_HTTP_BODY_H

namespace uh::cluster::ep::http {

class body {
public:
    virtual ~body() = default;
    virtual coro<std::size_t> read(std::span<char> dest) = 0;
};

} // namespace uh::cluster::ep::http

#endif
