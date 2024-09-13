#ifndef ETCD_LOCK_H
#define ETCD_LOCK_H
#include <etcd/Response.hpp>
#include <etcd/SyncClient.hpp>

namespace uh::cluster {

class etcd_lock {
public:
    explicit etcd_lock(etcd::SyncClient& client, const std::string& lock_key)
        : m_client(client),
          m_response(m_client.lock(lock_key)) {}

    ~etcd_lock() { m_client.unlock(m_response.lock_key()); }

private:
    etcd::SyncClient& m_client;
    etcd::Response m_response;
};

} // end namespace uh::cluster

#endif // ETCD_LOCK_H
