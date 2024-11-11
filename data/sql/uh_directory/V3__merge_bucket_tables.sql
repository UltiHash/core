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
    address BYTEA NOT NULL,
    size BIGINT NOT NULL,
    last_modified TIMESTAMP NOT NULL DEFAULT now(),
    etag TEXT DEFAULT NULL,
    mime TEXT DEFAULT NULL,
    UNIQUE (bucket_id, name)
);

-- Insert data from each bucket-specific table into __objects
DO $$
DECLARE bucket RECORD;
BEGIN
    FOR bucket IN SELECT id, name FROM __buckets LOOP
        EXECUTE format(
            'INSERT INTO __objects (bucket_id, name, address, size, last_modified, etag, mime)
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
DROP PROCEDURE IF EXISTS uh_check_bucket(REGCLASS);

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
-- uh_put_object(bucket, object, addr, etag) -- add an object with name `object`
-- described by `addr` to `bucket`. The object size is limited to 1GB.
--
DROP PROCEDURE uh_put_small_obj(REGCLASS, TEXT, BYTEA, BIGINT, TEXT, TEXT);

CREATE OR REPLACE PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
DECLARE bucket_id BIGINT;
BEGIN
    SELECT id INTO bucket_id FROM __buckets WHERE name = bucket;

    IF bucket_id IS NULL THEN
        RAISE EXCEPTION 'Bucket with name % does not exist.', bucket;
    END IF;

    EXECUTE 
        'INSERT INTO __objects (bucket_id, name, address, size, etag, mime)
         VALUES ($1, $2, $3, $4, $5, $6)
         ON CONFLICT (bucket_id, name) DO UPDATE
         SET address = EXCLUDED.address, size = EXCLUDED.size, etag = EXCLUDED.etag, mime = EXCLUDED.mime'
    USING bucket_id, object, address, size, etag, mime;
END
$$;

--
-- uh_get_object(bucket, object) -- retrieve the address portion of an object
-- identified by `bucket` and `object`. If the object contains no small address
-- data, NULL is returned.
--
DROP FUNCTION uh_get_object(REGCLASS, TEXT);

CREATE OR REPLACE FUNCTION uh_get_object(bucket TEXT, object TEXT)
    RETURNS TABLE (address BYTEA, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE 
        'SELECT address, size, last_modified, etag, mime 
         FROM __objects 
         WHERE bucket_id = (SELECT id FROM __buckets WHERE name = $1) AND name = $2'
    USING bucket, object;
END;
$$;


DROP FUNCTION uh_bucket_exists(regclass);

CREATE OR REPLACE FUNCTION uh_bucket_exists(bucket TEXT)
    RETURNS BOOLEAN
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    -- Now caller expects exception when it doesn't exist; no one use it's return value
    RETURN TRUE;
    -- RETURN EXISTS (SELECT 1 FROM __buckets WHERE name = bucket);
END;
$$;

--
-- uh_delete_bucket(bucket): delete bucket from system
--
DROP PROCEDURE IF EXISTS uh_delete_bucket(REGCLASS);

CREATE OR REPLACE PROCEDURE uh_delete_bucket(bucket TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    DELETE FROM __buckets WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist.', bucket;
    END IF;
END;
$$;
--
-- uh_delete_object(bucket, object_id): delete object
--
DROP PROCEDURE uh_delete_object(REGCLASS, TEXT);

CREATE OR REPLACE PROCEDURE uh_delete_object(bucket TEXT, object TEXT)
LANGUAGE plpgsql AS $$
DECLARE
    rows_deleted INT;
BEGIN
    EXECUTE 
        'DELETE FROM __objects 
         WHERE bucket_id = (SELECT id FROM __buckets WHERE name = $1) AND name = $2'
    USING bucket, object;

    GET DIAGNOSTICS rows_deleted = ROW_COUNT;
    IF rows_deleted = 0 THEN
        RAISE EXCEPTION 'Object "%s" in bucket "%s" does not exist.', object, bucket;
    END IF;
END;
$$;

--
-- uh_copy_object(bucket_src, object_src, bucket_dest, object_dest):
--
DROP PROCEDURE uh_copy_object(REGCLASS, TEXT, REGCLASS, TEXT);

CREATE OR REPLACE PROCEDURE uh_copy_object(bucket_src TEXT, object_src TEXT, bucket_dst TEXT, object_dst TEXT)
LANGUAGE plpgsql AS $$
DECLARE obj_record RECORD;
BEGIN
    SELECT address, size, last_modified, etag, mime
    INTO obj_record
    FROM __objects
    WHERE bucket_id = (SELECT id FROM __buckets WHERE name = bucket_src)
      AND name = object_src;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Object "%s" in bucket "%s" does not exist.', object_src, bucket_src;
    END IF;

    CALL uh_check_bucket(bucket_dst);

    EXECUTE 
        'INSERT INTO __objects (bucket_id, name, address, size, last_modified, etag, mime)
         VALUES ((SELECT id FROM __buckets WHERE name = $1), $2, $3, $4, $5, $6, $7)
         ON CONFLICT(bucket_id, name) DO UPDATE
         SET address = EXCLUDED.address, size = EXCLUDED.size,
             last_modified = EXCLUDED.last_modified,
             etag = EXCLUDED.etag, mime = EXCLUDED.mime'
    USING bucket_dst, object_dst, obj_record.address, obj_record.size, obj_record.last_modified, obj_record.etag, obj_record.mime;
END;
$$;

--
-- uh_list_objects(bucket): return all objects in `bucket`
--
DROP FUNCTION uh_list_objects(REGCLASS);

CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
    SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
    FROM __objects o
    JOIN __buckets b ON o.bucket_id = b.id
    WHERE b.name = bucket
    ORDER BY o.name;
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix`
--
DROP FUNCTION uh_list_objects(REGCLASS, TEXT);

CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT, prefix TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
    SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
    FROM __objects o
    JOIN __buckets b ON o.bucket_id = b.id
    WHERE b.name = bucket
      AND o.name LIKE prefix || '%'
    ORDER BY o.name;
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix` that are bigger than `lower_bound`
--
DROP FUNCTION uh_list_objects(bucket REGCLASS, prefix TEXT, lower_bound TEXT);

CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT, prefix TEXT, lower_bound TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
        FROM __objects o
        JOIN __buckets b ON o.bucket_id = b.id
        WHERE b.name = bucket
          AND o.name LIKE prefix || '%'
          AND (lower_bound = '' OR 
              (o.name > lower_bound AND NOT starts_with(o.name, lower_bound)))
        ORDER BY o.name;
END;
$$;


--
-- Returns 0 if the bucket has no object
--
CREATE OR REPLACE FUNCTION uh_bucket_size(bucket TEXT)
    RETURNS BIGINT
LANGUAGE plpgsql AS $$
DECLARE result BIGINT;
BEGIN
    SELECT SUM(o.size)
    INTO result
    FROM __objects o
    JOIN __buckets b ON o.bucket_id = b.id
    WHERE b.name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in __buckets table', bucket;
    END IF;

    RETURN COALESCE(result, 0);
END;
$$;


--
-- uh_bucket_policy(bucket): get the policy from a bucket
--
DROP FUNCTION uh_bucket_policy(REGCLASS);

CREATE OR REPLACE FUNCTION uh_bucket_policy(bucket TEXT)
    RETURNS TABLE(policy JSON)
LANGUAGE plpgsql AS $$
DECLARE policy_record JSON;
BEGIN
    -- Execute the query and save the result to a variable
    SELECT policy INTO policy_record
    FROM __buckets
    WHERE name = bucket;

    -- Check if the bucket was found
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in __buckets table', bucket;
    END IF;

    -- Return the policy
    RETURN QUERY SELECT policy_record;
END;
$$;

--
-- uh_bucket_set_policy(bucket): set the policy for a bucket
--
DROP PROCEDURE uh_bucket_set_policy(REGCLASS, JSON);

CREATE OR REPLACE PROCEDURE uh_bucket_set_policy(bucket TEXT, new_policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE __buckets
    SET policy = new_policy
    WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in __buckets table', bucket;
    END IF;
END;
$$;
