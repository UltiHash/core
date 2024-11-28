--
-- Lock access to an upload
--
CREATE OR REPLACE PROCEDURE uh_lock_upload(id TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    PERFORM pg_advisory_lock(hashtext(id));
END;
$$;

CREATE OR REPLACE PROCEDURE uh_unlock_upload(id TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    PERFORM pg_advisory_unlock(hashtext(id));
END;
$$;

