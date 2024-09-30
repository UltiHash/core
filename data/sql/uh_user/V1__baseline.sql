-- ------------------------------------------------------------------------
--
-- Database tables
--

--
-- Table of users
--
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name TEXT PRIMARY KEY,
    password TEXT NULL,
    policy JSON DEFAULT NULL,
    arn TEXT
);

CREATE TABLE keys (
    username TEXT NOT NULL REFERENCES users ON DELETE CASCADE,
    access_key TEXT UNIQUE NOT NULL,
    secret_key TEXT NOT NULL,
    session_token TEXT DEFAULT NULL,
    expires TIMESTAMP DEFAULT NULL
);
CREATE INDEX expires_idx ON keys (expires);

-- ------------------------------------------------------------------------
--
-- Database functions for controlling UH users
--

--
-- uh_add_user(username, password, arn) -- create a new
--   user that will expire after a given time and return it's id
--
CREATE OR REPLACE FUNCTION uh_add_user(username TEXT, password TEXT, arn TEXT) RETURNS TEXT
LANGUAGE plpgsql AS $$
DECLARE id TEXT;
BEGIN
    EXECUTE format('INSERT INTO users (name, password, arn) VALUES (%L, %L, %L) RETURNING id',
        username, password, arn) INTO id;
    RETURN id;
END;
$$;

--
-- uh_remove_user(username) -- remove a user completely
--
CREATE OR REPLACE PROCEDURE uh_remove_user(username TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE FROM users WHERE name = %L', username);
END;
$$;

--
-- uh_set_user_policy(username, policy) -- set user policy for the referenced
-- user. Set policy to `NULL` to remove it.
--
CREATE OR REPLACE PROCEDURE uh_set_user_policy(username TEXT, policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('UPDATE users SET policy = %L WHERE name = %L', policy, username);
END;
$$;

--
-- uh_query_user(username) -- retrieve user information suitable for
-- authenticating a user.
--
CREATE OR REPLACE FUNCTION uh_query_user(username TEXT)
    RETURNS TABLE (id UUID, password TEXT, policy JSON, arn TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT id, password, policy, arn FROM users WHERE name = %L',
        username);
END;
$$;

--
-- uh_list_users() -- retrieve a list of all usernames
--
CREATE OR REPLACE FUNCTION uh_list_users()
    RETURNS TABLE (username TEXT)
LANGUAGE SQL AS $$
    SELECT name FROM users;
$$;


--
-- uh_add_user_key(username, access_key, secret_key, session_token, expires)
-- add an access key for a given user with secret and session token. The key can
-- be given an expiration timestamp.
--
CREATE OR REPLACE PROCEDURE uh_add_user_key(username TEXT, access_key TEXT,
    secret_key TEXT, session_token TEXT, expires TIMESTAMP)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('
        INSERT INTO keys (username, access_key, secret_key, session_token, expires)
        VALUES (%L, %L, %L, %L, %L)',
        username, access_key, secret_key, session_token, expires);
END;
$$;

--
-- uh_query_key(access_key) -- return info associated with access key
--
CREATE OR REPLACE FUNCTION uh_query_key(access_key TEXT)
    RETURNS TABLE(id UUID, username TEXT, secret_key TEXT, session_token TEXT, expires TIMESTAMP, policy JSON, arn TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('
        SELECT id, username, secret_key, session_token, expires, policy, arn FROM keys
        JOIN users ON username = name WHERE access_key = %L AND (expires >= now() OR expires IS NULL)', access_key);
END;
$$;

--
-- uh_remove_key(access_key) -- delete an access key
--
CREATE OR REPLACE PROCEDURE uh_remove_key(access_key TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE from keys WHERE access_key = %L', access_key);
END;
$$;

--
-- uh_remove_expired_keys(expired_before) -- remove all keys that have expired
-- before the given timestamp
--
CREATE OR REPLACE PROCEDURE uh_remove_expired_keys(expired_before TIMESTAMP)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE FROM keys WHERE expires <= %L', expired_before);
END;
$$;
