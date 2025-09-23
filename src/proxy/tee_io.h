#pragma once

#include <common/telemetry/trace/awaitable_operators.h>
#include <common/types/common_types.h>

namespace uh::cluster::proxy {

template <typename T, typename U> class tee {
public:
    tee(T& t, U& u)
        : m_t{t},
          m_u{u} {}

    coro<void> put(std::span<const char> sv) {
        using boost::asio::experimental::awaitable_operators::operator&&;

        if (sv.size() == 0) {
            co_return;
        }
        co_await (m_t.put(sv) && m_u.put(sv));
    }

private:
    T& m_t;
    U& m_u;
};

} // namespace uh::cluster::proxy
