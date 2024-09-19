-- ------------------------------------------------------------------------
--
-- Database tables
--

--
-- Table of users
--
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    access_key TEXT UNIQUE NOT NULL,
    secret_key TEXT NOT NULL,
    session_token TEXT DEFAULT NULL,
    expires TIMESTAMP DEFAULT NULL,
    policy JSON DEFAULT NULL
);

-- ------------------------------------------------------------------------
--
-- Database functions for controlling the multipart state
--

--
-- uh_create_user(access_key, secret_key) -- create a new user and return it's
-- id
--
CREATE OR REPLACE FUNCTION uh_create_user(access_key TEXT, secret_key TEXT) RETURNS TEXT
LANGUAGE plpgsql AS $$
DECLARE id TEXT;
BEGIN
    EXECUTE format('INSERT INTO users (access_key, secret_key) VALUES (%L, %L) RETURNING id',
        access_key, secret_key) INTO id;
    RETURN id;
END;
$$;

--
-- uh_create_temporary_user(access_key, secret_key, session_token, expires) -- create a new
--   user that will expire after a given time and return it's id
--
CREATE OR REPLACE FUNCTION uh_create_temporary_user(access_key TEXT, secret_key TEXT, session_token TEXT, expires TIMESTAMP) RETURNS TEXT
LANGUAGE plpgsql AS $$
DECLARE id TEXT;
BEGIN
    EXECUTE format('INSERT INTO users (access_key, secret_key, session_token, expires) VALUES (%L, %L, %L, %L) RETURNING id',
        access_key, secret_key, session_token, expires) INTO id;
    RETURN id;
END;
$$;

--
-- uh_delete_user(access_key) -- remove a user completely
--
CREATE OR REPLACE PROCEDURE uh_delete_user(access_key TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE FROM users WHERE access_key = %L', access_key);
END;
$$;

--
-- uh_set_user_policy(access_key, policy) -- set user policy for the referenced
-- user
--
CREATE OR REPLACE PROCEDURE uh_set_user_policy(access_key TEXT, policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('UPDATE users SET policy = %L WHERE access_key = %L', policy, access_key);
END;
$$;

--
-- uh_remove_expired_users() -- remove all users that have expired
--
CREATE OR REPLACE PROCEDURE uh_remove_expired_users()
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE FROM users WHERE expires <= now()');
END;
$$;

--
-- uh_query_user(access_key, session_token) -- retrieve user information
-- suitable for authenticating a user. Returns secret_key and policy.
--
CREATE OR REPLACE FUNCTION uh_query_user(access_key TEXT, session_token TEXT)
    RETURNS TABLE (secret_key TEXT, policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT secret_key, policy FROM users WHERE access_key = %L and session_token = %L and expires > now()',
        access_key, session_token);
END;
$$;
