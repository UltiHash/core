#include "client_pool.h"

#include <logging/logging_boost.h>


namespace uh::protocol
{

// ---------------------------------------------------------------------

client_pool::handle::~handle()
{
    if (owning)
    {
        m_pool.put_back(std::move(m_client));
    }
}

// ---------------------------------------------------------------------

client* client_pool::handle::operator->()
{
    return m_client.get();
}

// ---------------------------------------------------------------------

client_pool::handle::handle(client_pool& pool, std::unique_ptr<client>&& c)
    : m_pool(pool),
      m_client(std::move(c))
{
}

// ---------------------------------------------------------------------

client_pool::client_pool(std::unique_ptr<util::factory<client>>&& factory, std::size_t pool_size)
    : m_factory(std::move(factory)),
      m_pool_size(pool_size),
      m_available_connection(pool_size)
{
    while (m_clients.size() < m_pool_size)
    {
        auto client = m_factory->create();

        if(m_server_uuid.empty()) {
            m_server_uuid = client.get()->get_server_uuid();
        } else {
            if(m_server_uuid != client.get()->get_server_uuid()) {
                THROW(uh::util::illegal_state, "Mismatching server uuids detected in client pool. "
                                               "All clients in a pool must connect to the same server.");
            }
        }

        m_clients.push_back(std::move(client));
    }
}

// ---------------------------------------------------------------------

client_pool::handle client_pool::get()
{
    std::unique_lock lk(m_mutex);

    while (m_available_connection == 0)
    {
        m_cv.wait(lk);
    }

    m_available_connection--;
    if (!m_clients.empty())
    {
        auto client = std::move(m_clients.front());
        m_clients.pop_front();
        return {*this, std::move(client)};
    }
    else
    {
        return {*this, m_factory->create()};
    }

}

// ---------------------------------------------------------------------

void client_pool::quit_all(const std::string& reason)
{
    std::lock_guard lk(m_mutex);

    while (!m_clients.empty())
    {
        auto client = std::move(m_clients.front());
        m_clients.pop_front();

        client->quit(reason);
    }
}

// ---------------------------------------------------------------------

void client_pool::put_back(std::unique_ptr<client> c)
{
    std::unique_lock lk(m_mutex);

    if (c->valid())
    {
        m_clients.push_back(std::move(c));
    }

    m_available_connection++;

    lk.unlock();
    m_cv.notify_one();

}

// ---------------------------------------------------------------------

std::string& client_pool::get_server_uuid() {
    return m_server_uuid;
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
