#ifndef CORE_ENTRYPOINT_USER_DB_H
#define CORE_ENTRYPOINT_USER_DB_H

#include "common/db/db.h"
#include "user.h"

namespace uh::cluster::ep::user {

class db {
public:
    db(boost::asio::io_context& ioc, const uh::cluster::db::config& cfg);

    struct user_info {
        std::string secret_key;
        std::optional<std::string> policy;
    };

    /**
     * Find a user using the access_key.
     *
     * @param key access key of the user
     */
    coro<user_info> find(std::string_view key);

    /**
     * Add a user and return the access key.
     *
     * @param key the key to access the user
     * @param secret the secret to assign
     * @param sts sts token
     * @param ttl number of seconds until the account expires (optional)
     */
    coro<void> add(const std::string& key, const std::string& secret,
                   const std::string& sts, std::optional<std::size_t> ttl);

    /**
     * Remove user from the DB.
     *
     * @param key the user's access key
     */
    coro<void> remove(const std::string& key);

    /**
     * Set the policy for a user
     *
     * @param key access_key access key identifying the user
     * @param policy the policy to set for the user or `std::nullopt` to disable
     *               the policy
     */
    coro<void> policy(const std::string& key,
                      std::optional<std::string> policy);

    /**
     * Get list of users in the database
     */
    coro<std::list<std::string>> entries();

    /**
     * Remove all expired users.
     */
    coro<void> remove_expired();

private:
    pool<cluster::db::connection> m_db;
};

} // namespace uh::cluster::ep::user

#endif
