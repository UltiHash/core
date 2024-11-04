--
-- Creates a lock on the specified object until the current transaction
-- is left. The lock is of type `FOR NO KEY UPDATE`.
--
CREATE OR REPLACE PROCEDURE uh_lock_object(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT id FROM %s WHERE name = %L FOR NO KEY UPDATE',
        bucket, key);
END
$$;
