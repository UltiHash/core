-- ------------------------------------------------------------------------
--
-- Database tables
--

-- For better performance and data integrity
ALTER TABLE __buckets
    ADD PRIMARY KEY (id);

-- To set unique constraint on name column
ALTER TABLE __buckets
    ADD CONSTRAINT unique_name UNIQUE (name);


--
-- The table "__objects" lists all available objects across all buckets
--
CREATE TABLE __objects (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    bucket_id BIGINT NOT NULL REFERENCES __buckets(id) ON DELETE RESTRICT,
    name TEXT NOT NULL,
    small BYTEA,
    size BIGINT NOT NULL,
    last_modified TIMESTAMP NOT NULL DEFAULT now(),
    etag TEXT,
    mime TEXT,
    UNIQUE (bucket_id, name)
);

-- Insert data from each bucket-specific table into __objects
DO $$
DECLARE
    bucket RECORD;
BEGIN
    FOR bucket IN SELECT id, name FROM __buckets LOOP
        EXECUTE format(
            'INSERT INTO __objects (bucket_id, name, small, size, last_modified, etag, mime)
             SELECT %L, name, small, size, last_modified, etag, mime
             FROM %I',
            bucket.id,  -- bucket_id from __buckets
            bucket.name -- dynamic table name from __buckets.name
        );
    EXECUTE format('DROP TABLE IF EXISTS %I', bucket.name);
    END LOOP;
END $$;

-- ------------------------------------------------------------------------
--
-- Database functions for controlling the directory
--

--
-- uh_check_bucket(bucket) -- raise an exception when the bucket doesn't exist
--
DROP PROCEDURE IF EXISTS uh_check_bucket(REGCLASS)

CREATE OR REPLACE PROCEDURE uh_check_bucket(bucket TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    PERFORM 1 FROM __buckets WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in __buckets table', bucket;
    END IF;
END
$$;

--
-- uh_create_bucket(bucket) -- create a new bucket
--
CREATE OR REPLACE PROCEDURE uh_create_bucket(bucket TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    INSERT INTO __buckets (name) VALUES (bucket);
END
$$;

--
-- uh_put_small_obj(bucket, key, addr, etag) -- add an object with name `key`
-- described by `addr` to `bucket`. The object size is limited to 1GB.
--
DROP PROCEDURE uh_put_small_obj(REGCLASS, TEXT, BYTEA, BIGINT, TEXT, TEXT)

CREATE OR REPLACE PROCEDURE uh_put_small_obj(bucket_name TEXT, name TEXT, addr BYTEA, size BIGINT, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
DECLARE
    bucket_id BIGINT;
BEGIN
    SELECT id INTO bucket_id FROM __buckets WHERE name = bucket_name;

    IF bucket_id IS NULL THEN
        RAISE EXCEPTION 'Bucket with name % does not exist.', bucket_name;
    END IF;

    EXECUTE 
        'INSERT INTO __objects (bucket_id, name, small, size, etag, mime)
         VALUES ($1, $2, $3, $4, $5, $6)
         ON CONFLICT (bucket_id, name) DO UPDATE
         SET small = EXCLUDED.small, size = EXCLUDED.size, etag = EXCLUDED.etag, mime = EXCLUDED.mime'
    USING bucket_id, name, addr, size, etag, mime;
END
$$;

--
-- uh_get_object(bucket, key) -- retrieve the address portion of an object
-- identified by `bucket` and `key`. If the object contains no small address
-- data, NULL is returned.
--
DROP FUNCTION uh_get_object(REGCLASS, TEXT)

CREATE OR REPLACE FUNCTION uh_get_object(bucket TEXT, key TEXT)
    RETURNS TABLE (small BYTEA, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE 
        'SELECT small, size, last_modified, etag, mime 
         FROM __objects 
         WHERE bucket_id = (SELECT id FROM __buckets WHERE name = $1) AND name = $2'
    USING bucket, key
    );
END;
$$;

--
-- uh_bucket_exists(bucket): return true if the bucket exists
--
CREATE OR REPLACE FUNCTION uh_bucket_exists(bucket TEXT)
    RETURNS BOOLEAN
LANGUAGE plpgsql AS $$
BEGIN
    RETURN EXISTS (SELECT 1 FROM __buckets WHERE name = bucket);
END;
$$;

--
-- uh_delete_bucket(bucket): delete bucket from system
--
DROP PROCEDURE IF EXISTS uh_delete_bucket(REGCLASS);

CREATE OR REPLACE PROCEDURE uh_delete_bucket(bucket TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE FROM __buckets WHERE name = %L', bucket);
END;
$$;

--
-- uh_delete_object(bucket, object_id): delete object
--
DROP PROCEDURE uh_delete_object(REGCLASS, TEXT)

CREATE OR REPLACE PROCEDURE uh_delete_object(bucket TEXT, key TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    -- Perform the delete operation
    EXECUTE 
        'DELETE FROM __objects 
         WHERE bucket_id = (SELECT id FROM __buckets WHERE name = $1) AND name = $2'
    USING bucket, key;

    -- Optionally, check if the delete operation was successful
    IF NOT FOUND THEN
        RAISE NOTICE 'Object with key "%s" in bucket "%s" does not exist.', key, bucket;
    END IF;
END;
$$;

--
-- uh_copy_object(bucket_src, key_src, bucket_dest, key_dest):
--
DROP PROCEDURE uh_copy_object(REGCLASS, TEXT, REGCLASS, TEXT)

CREATE OR REPLACE PROCEDURE uh_copy_object(bucket_src TEXT, key_src TEXT, bucket_dst TEXT, key_dst TEXT)
LANGUAGE plpgsql AS $$
DECLARE
    obj_record RECORD;
BEGIN
    SELECT small, size, last_modified, etag, mime
    INTO obj_record
    FROM __objects
    WHERE bucket_id = (SELECT id FROM __buckets WHERE name = bucket_src)
      AND name = key_src;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Object with key "%s" in bucket "%s" does not exist.', key_src, bucket_src;
    END IF;

    PERFORM uh_check_bucket(bucket_dst);

    EXECUTE 
        'INSERT INTO __objects (bucket_id, name, small, size, last_modified, etag, mime)
         VALUES ((SELECT id FROM __buckets WHERE name = $1), $2, $3, $4, $5, $6, $7)
         ON CONFLICT(bucket_id, name) DO UPDATE
         SET small = EXCLUDED.small, size = EXCLUDED.size,
             last_modified = EXCLUDED.last_modified,
             etag = EXCLUDED.etag, mime = EXCLUDED.mime'
    USING bucket_dst, key_dst, obj_record.small, obj_record.size, obj_record.last_modified, obj_record.etag, obj_record.mime;
END;
$$;

--
-- uh_list_objects(bucket): return all objects in `bucket`
--
DROP FUNCTION uh_list_objects(REGCLASS)

CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY EXECUTE format('SELECT id, name, size, last_modified, etag, mime FROM %s ORDER BY name', bucket);
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix`
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket REGCLASS, prefix TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY EXECUTE format('SELECT id, name, size, last_modified, etag, mime FROM %s WHERE name LIKE %L ORDER BY name', bucket, prefix || '%');
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix` that are bigger than `lower_bound`
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket REGCLASS, prefix TEXT, lower_bound TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    IF lower_bound = '' THEN
        RETURN QUERY EXECUTE
            format('SELECT id, name, size, last_modified, etag, mime FROM %s WHERE name LIKE %L AND name > %L ORDER BY name',
                bucket, prefix || '%', lower_bound);
    ELSE
        RETURN QUERY EXECUTE
            format('SELECT id, name, size, last_modified, etag, mime FROM %s WHERE name LIKE %L AND name > %L AND NOT starts_with(name, %L) ORDER BY name',
                bucket, prefix || '%', lower_bound, lower_bound);
    END IF;
END;
$$;

--
--
--
--
CREATE OR REPLACE FUNCTION uh_bucket_size(bucket REGCLASS)
    RETURNS BIGINT
LANGUAGE plpgsql AS $$
DECLARE result BIGINT;
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('SELECT sum(size) FROM %s', bucket) INTO result;
    RETURN result;
END;
$$;


--
-- uh_bucket_policy(bucket): get the policy from a bucket
--
DROP FUNCTION uh_bucket_policy(REGCLASS)

CREATE OR REPLACE FUNCTION uh_bucket_policy(bucket TEXT)
    RETURNS TABLE(policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT policy FROM __buckets WHERE name = %L', bucket);
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket name % does not exist in __buckets table.', bucket_name;
    END IF;
END;
$$;

--
-- uh_bucket_set_policy(bucket): set the policy for a bucket
--
DROP PROCEDURE uh_bucket_set_policy(REGCLASS, JSON)

CREATE OR REPLACE PROCEDURE uh_bucket_set_policy(bucket REGCLASS, policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('UPDATE __buckets SET policy = %L WHERE name = %L', policy, bucket);

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket name % does not exist in __buckets table.', bucket_name;
    END IF;
END;
$$;

