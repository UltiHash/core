#ifndef CORE_ENTRYPOINT_USER_DB_H
#define CORE_ENTRYPOINT_USER_DB_H

#include "common/crypto/scrypt.h"
#include "common/db/db.h"
#include "user.h"

namespace uh::cluster::ep::user {

class db {
public:
    db(boost::asio::io_context& ioc, const uh::cluster::db::config& cfg);

    /**
     * Find a user using the access_key.
     *
     * @param key access key of the user
     */
    coro<user> find(std::string_view key);

    /**
     * Find a user using username and password.
     */
    coro<user> find(std::string id, std::string pass);

    /**
     * Add a new user to database
     *
     * @param username user name
     * @param password
     * @param arn user arn
     */
    coro<void> add_user(const std::string& name, const std::string& password,
                        std::optional<std::string> arn);

    /**
     * Add a key to a user
     *
     * @param name
     * @param key the key to access the user
     * @param secret the secret to assign
     * @param sts sts token
     * @param ttl number of seconds until the account expires (optional)
     */
    coro<void> add_key(const std::string& name, const std::string& key,
                       const std::string& secret,
                       std::optional<std::string> sts,
                       std::optional<std::size_t> ttl);

    /**
     * Remove access key from the DB.
     *
     * @param key the user's access key
     */
    coro<void> remove_key(const std::string& key);

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
     * Remove all expired keys.
     */
    coro<void> remove_expired(std::size_t seconds);

private:
    pool<cluster::db::connection> m_db;
    scrypt m_crypt;
};

} // namespace uh::cluster::ep::user

#endif
