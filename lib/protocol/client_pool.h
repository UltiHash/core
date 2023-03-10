#ifndef PROTOCOL_CLIENT_POOL_H
#define PROTOCOL_CLIENT_POOL_H

#include <util/factory.h>
#include "client.h"

#include <condition_variable>
#include <list>
#include <mutex>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class client_pool
{
public:

    class handle
    {
    public:
        ~handle();
        client* operator->();

    private:
        friend client_pool;
        handle(client_pool& pool, std::unique_ptr<client>&& c);

        client_pool& m_pool;
        std::unique_ptr<client> m_client;

    };

    client_pool(std::unique_ptr<util::factory<client>>&& factory,
                std::size_t pool_size);

    handle get();

private:
    friend class handle;
    void put_back(std::unique_ptr<client> c);

    std::list<std::unique_ptr<client>> m_clients;
    std::unique_ptr<util::factory<client>> m_factory;
    std::size_t m_pool_size;
    std::size_t m_available_connection;

    std::mutex m_mutex;
    std::condition_variable m_cv;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
