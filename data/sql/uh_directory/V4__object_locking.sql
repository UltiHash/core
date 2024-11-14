--
-- Creates a lock on the specified object until `un_unlock_object` is called or
-- the database session is closed. The lock is an advisory lock on a hash of the
-- strings `bucket` and `key`.
--
CREATE OR REPLACE PROCEDURE uh_lock_object(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_lock(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;

--
-- Releases a previously obtained lock for the specified object.
--
CREATE OR REPLACE PROCEDURE uh_unlock_object(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_unlock(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;

--
-- Creates a shared lock on the specified object until `un_unlock_object_shared`
-- is called or the database session is close. The lock is a shared advisory
-- lock on a hash of the strings `bucket` and `key`.
--
CREATE OR REPLACE PROCEDURE uh_lock_object_shared(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_lock_shared(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;

--
-- Releases a previously obtained shared lock for the specified object.
--
CREATE OR REPLACE PROCEDURE uh_unlock_object_shared(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_unlock_shared(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;
