--
-- Following functions define constants to be used to handle status
--
CREATE OR REPLACE FUNCTION status_normal() RETURNS INT
LANGUAGE sql IMMUTABLE PARALLEL SAFE AS
    'SELECT 0';

CREATE OR REPLACE FUNCTION status_deleted() RETURNS INT
LANGUAGE sql IMMUTABLE PARALLEL SAFE AS
    'SELECT 1';

CREATE OR REPLACE FUNCTION status_collected() RETURNS INT
LANGUAGE sql IMMUTABLE PARALLEL SAFE AS
    'SELECT 2';

ALTER TABLE __objects
    ADD COLUMN refs INTEGER NOT NULL DEFAULT 0,
    ADD COLUMN status INTEGER NOT NULL DEFAULT status_normal(),
    ADD COLUMN version UUID NOT NULL DEFAULT gen_random_uuid(),
    DROP CONSTRAINT __objects_bucket_id_name_key,
    ADD UNIQUE (bucket_id, name, version);

ALTER TABLE __buckets
    ADD COLUMN status INTEGER NOT NULL DEFAULT status_normal();

--
-- Get next object to delete
--
CREATE OR REPLACE FUNCTION uh_next_deleted() RETURNS TABLE(id BIGINT, address BYTEA)
LANGUAGE plpgsql AS $$
DECLARE target_id BIGINT;
DECLARE target_address BYTEA;
BEGIN
    SELECT o.id, o.address FROM __objects o WHERE status = status_deleted() AND refs = 0
        INTO target_id, target_address;

    IF NOT FOUND THEN
        RETURN;
    END IF;

    UPDATE __objects o SET status = status_collected() WHERE o.id = target_id;

    RETURN QUERY SELECT target_id, target_address;
END
$$;

--
-- Remove deleted buckets
--
CREATE OR REPLACE PROCEDURE uh_clear_deleted_buckets()
LANGUAGE plpgsql AS $$
BEGIN
    DELETE FROM __buckets b WHERE status = status_deleted()
        AND NOT EXISTS (SELECT 1 FROM __objects o WHERE o.bucket_id = b.id);
END
$$;

--
-- Remove object by id
--
CREATE OR REPLACE PROCEDURE uh_delete_object_by_id(target_id BIGINT)
LANGUAGE plpgsql AS $$
BEGIN
    DELETE FROM __objects WHERE id = target_id;
END
$$;

--
-- Increment the reference for an object
--
CREATE OR REPLACE PROCEDURE uh_inc_reference(target_id BIGINT)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE __objects SET refs = refs + 1 WHERE id = target_id AND status = status_normal();

    IF NOT FOUND THEN
        RAISE EXCEPTION 'object not found';
    END IF;
END
$$;

--
-- Decrement the reference for an object
--
CREATE OR REPLACE PROCEDURE uh_dec_reference(target_id BIGINT)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE __objects SET refs = refs - 1 WHERE id = target_id;
END
$$;

--
-- Change put_object to set status = 1 for entries of a given bucket/object that
-- are already there and have status = 0.
--
CREATE OR REPLACE PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
DECLARE bucket_id BIGINT;
BEGIN
    SELECT id INTO bucket_id FROM __buckets WHERE name = bucket;

    IF bucket_id IS NULL THEN
        RAISE EXCEPTION 'Bucket with name % does not exist.', bucket;
    END IF;

    EXECUTE 'UPDATE __objects SET status = status_deleted() WHERE bucket_id = $1 AND name = $2'
        USING bucket_id, object;

    EXECUTE 'INSERT INTO __objects (bucket_id, name, address, size, etag, mime)
        VALUES ($1, $2, $3, $4, $5, $6)'
        USING bucket_id, object, address, size, etag, mime;
END
$$;

--
-- uh_get_object(bucket, object) -- retrieve the data stored for an object
-- identified by `bucket` and `object`.
--
DROP FUNCTION uh_get_object(bucket TEXT, object TEXT);
CREATE OR REPLACE FUNCTION uh_get_object(bucket TEXT, object TEXT)
    RETURNS TABLE (id BIGINT, address BYTEA, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE
        'SELECT id, address, size, last_modified, etag, mime
         FROM __objects
         WHERE bucket_id = (SELECT id FROM __buckets WHERE name = $1) AND name = $2
         AND status = status_normal() ORDER BY id DESC'
    USING bucket, object;
END;
$$;

--
-- uh_delete_bucket(bucket): delete bucket from system
--
CREATE OR REPLACE PROCEDURE uh_delete_bucket(bucket TEXT)
LANGUAGE plpgsql AS $$
DECLARE object_count INT;
BEGIN
    SELECT count(1) INTO object_count FROM __objects
    WHERE bucket_id = (SELECT id FROM __buckets WHERE name = bucket) AND status = status_normal();

    IF object_count != 0 THEN
        RAISE EXCEPTION 'Bucket "%" is not empty.', bucket;
    END IF;

    UPDATE __buckets SET status = status_deleted(), name = gen_random_uuid()
        WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist.', bucket;
    END IF;
END;
$$;

--
-- uh_delete_object(bucket, object_id): delete object
--
CREATE OR REPLACE PROCEDURE uh_delete_object(bucket TEXT, object TEXT)
LANGUAGE plpgsql AS $$
DECLARE
    rows_deleted INT;
BEGIN
    EXECUTE
        'UPDATE __objects SET status = status_deleted()
         WHERE bucket_id = (SELECT id FROM __buckets WHERE name = $1) AND name = $2
         AND status = status_normal()'
    USING bucket, object;

    GET DIAGNOSTICS rows_deleted = ROW_COUNT;
    IF rows_deleted = 0 THEN
        RAISE EXCEPTION 'Object "%" in bucket "%" does not exist.', object, bucket;
    END IF;
END;
$$;

-- not required anymore
DROP PROCEDURE uh_copy_object(bucket_src TEXT, object_src TEXT, bucket_dst TEXT, object_dst TEXT);

--
-- uh_list_objects(bucket): return all objects in `bucket`
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
    SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
    FROM __objects o
    JOIN __buckets b ON o.bucket_id = b.id
    WHERE b.name = bucket AND o.status = status_normal()
    ORDER BY o.name;
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix`
--
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
      AND o.name LIKE prefix || '%' AND o.status = status_normal()
    ORDER BY o.name;
END;
$$;

--
-- uh_list_objects(bucket, prefix, lower_bound): return all objects in `bucket` with a
-- given `prefix` that are bigger than `lower_bound`
--
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
          AND o.status = status_normal()
          AND (lower_bound = '' OR
              (o.name > lower_bound AND NOT starts_with(o.name, lower_bound)))
        ORDER BY o.name;
END;
$$;

--
-- Returns 0 if the bucket has no object
--
DROP FUNCTION uh_bucket_size(bucket TEXT);
CREATE OR REPLACE FUNCTION uh_data_size() RETURNS BIGINT
LANGUAGE plpgsql AS $$
DECLARE result BIGINT;
BEGIN
    SELECT SUM(size) INTO result FROM __objects WHERE status = status_normal();
    RETURN COALESCE(result, 0);
END;
$$;
