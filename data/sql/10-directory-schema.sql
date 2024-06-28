\c uh_directory;

-- ------------------------------------------------------------------------
--
-- Database tables
--

--
-- The table `__buckets` lists all available buckets in the system.
--
CREATE TABLE __buckets (
    id bigint GENERATED ALWAYS AS IDENTITY,
    name VARCHAR(64) NOT NULL
);

-- ------------------------------------------------------------------------
--
-- Database helper functions
--

--
-- rel_name(table) -- return textual name of table
--
CREATE OR REPLACE FUNCTION rel_name(regclass)
    RETURNS name LANGUAGE SQL AS $$
    SELECT relname FROM pg_class WHERE OID = $1
$$;

-- ------------------------------------------------------------------------
--
-- Database functions for controlling the directory
--

--
-- uh_check_bucket(bucket) -- raise an exception when the bucket references
-- internal tables
--
CREATE OR REPLACE PROCEDURE uh_check_bucket(bucket regclass)
LANGUAGE plpgsql AS $$
BEGIN
    IF bucket = '__buckets'::regclass THEN
        RAISE 'cannot create bucket: %', bucket;
    END IF;
END
$$;

CREATE OR REPLACE PROCEDURE uh_check_bucket(bucket text)
LANGUAGE plpgsql AS $$
BEGIN
    IF bucket LIKE '%__buckets%' THEN
        RAISE 'cannot create bucket: %', bucket;
    END IF;
END
$$;

--
-- uh_create_bucket(bucket) -- create a new bucket
--
CREATE OR REPLACE PROCEDURE uh_create_bucket(bucket text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE 'CREATE table ' || quote_ident(bucket) || ' (
        id bigint GENERATED ALWAYS AS IDENTITY,
        name TEXT NOT NULL PRIMARY KEY,
        small BYTEA DEFAULT NULL,
        large OID DEFAULT NULL,
        size BIGINT NOT NULL,
        last_modified TIMESTAMP NOT NULL DEFAULT now(),
        etag TEXT DEFAULT NULL
    )';

    INSERT INTO __buckets (name) VALUES (bucket);
END
$$;

--
-- uh_put_small_obj(bucket, key, addr, etag) -- add an object with name `key`
-- described by `addr` to `bucket`. The object size is limited to 1GB.
--
CREATE OR REPLACE PROCEDURE uh_put_small_obj(bucket regclass, key text, addr BYTEA, size bigint, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('
        INSERT INTO %s ("name", "small", "size", "etag") VALUES(%L, %L, %L, %L)
        ON CONFLICT ("name") DO UPDATE SET "small" = %L, "size"= %L, "etag"= %L',
        bucket, key, addr, size, etag, addr, size, etag);
END
$$;

--
-- uh_put_large_obj(bucket, key, addr) -- add an object with name `key`
-- described by `addr` to `bucket`. The object data must be written using
-- PostGre's large object facility. The resulting OID is passed to this
-- function as addr.
--
CREATE OR REPLACE PROCEDURE uh_put_large_obj(bucket regclass, key text, addr oid, size bigint)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('INSERT INTO %s ("name", "large", "size") VALUES(%L, %L, %L)', bucket, key, addr, size);
END
$$;

--
-- uh_get_object(bucket, key) -- retrieve the address portion of an object
-- identified by `bucket` and `key`. If the object contains no small address
-- data, NULL is returned.
--
CREATE OR REPLACE FUNCTION uh_get_object(bucket regclass, key text)
    RETURNS TABLE (small BYTEA, large oid, size BIGINT, last_modified TIMESTAMP, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY EXECUTE format('SELECT small, large, size, last_modified, etag FROM %s WHERE name = %L', bucket, key);
END;
$$;

--
-- uh_bucket_exists(bucket): return true if the bucket exists
--
CREATE OR REPLACE FUNCTION uh_bucket_exists(bucket regclass) RETURNS BOOLEAN
LANGUAGE plpgsql AS $$
DECLARE result BOOLEAN;
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('SELECT (EXISTS (SELECT count(1) FROM %s))', bucket)
        INTO result;
    RETURN result;
END;
$$;

--
-- uh_delete_bucket(bucket): delete bucket from system
--
CREATE OR REPLACE PROCEDURE uh_delete_bucket(bucket regclass)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('DELETE FROM __buckets WHERE name = %L', rel_name(bucket));
    EXECUTE format('DROP TABLE %s', bucket);
END;
$$;

--
-- uh_delete_object(bucket, object_id): delete object
--
CREATE OR REPLACE PROCEDURE uh_delete_object(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('DELETE FROM %s WHERE name = %L', bucket, key);
END;
$$;

--
-- uh_copy_object(bucket_src, key_src, bucket_dest, key_dest):
--
CREATE OR REPLACE PROCEDURE uh_copy_object(bucket_src regclass, key_src text,
    bucket_dst regclass, key_dst text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket_src);
    CALL uh_check_bucket(bucket_dst);

    EXECUTE format('
        INSERT INTO %s (name, small, size, last_modified, etag)
        SELECT %L, small, size, last_modified, etag FROM %s WHERE name = %L
        ON CONFLICT("name") DO UPDATE SET small = EXCLUDED.small, size = EXCLUDED.size, last_modified = EXCLUDED.last_modified, etag = EXCLUDED.etag',
        bucket_dst, key_dst, bucket_src, key_src);
END;
$$;

--
-- uh_copy_object_ifmatch(bucket_src, key_src, bucket_dest, key_dest, etag):
--
CREATE OR REPLACE PROCEDURE uh_copy_object_ifmatch(bucket_src regclass, key_src text,
    bucket_dst regclass, key_dst text, etag text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket_src);
    CALL uh_check_bucket(bucket_dst);

    EXECUTE format('
        INSERT INTO %s (name, small, size, last_modified, etag)
        SELECT %L, small, size, last_modified, etag FROM %s WHERE name = %L AND etag = %L
        ON CONFLICT("name") DO UPDATE SET small = EXCLUDED.small, size = EXCLUDED.size, last_modified = EXCLUDED.last_modified, etag = EXCLUDED.etag',
        bucket_dst, key_dst, bucket_src, key_src, etag);
END;
$$;

--
-- uh_list_buckets(): list all buckets
--
CREATE OR REPLACE FUNCTION uh_list_buckets() RETURNS TABLE(name text)
LANGUAGE SQL AS 'SELECT name FROM __buckets;';

--
-- uh_list_objects(bucket): return all objects in `bucket`
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket regclass)
    RETURNS TABLE(id bigint, name text, size bigint, last_modified TIMESTAMP, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY EXECUTE format('SELECT id, name, size, last_modified, etag FROM %s ORDER BY name', bucket);
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix`
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket regclass, prefix text)
    RETURNS TABLE(id bigint, name text, size bigint, last_modified TIMESTAMP, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY EXECUTE format('SELECT id, name, size, last_modified, etag FROM %s WHERE name LIKE %L ORDER BY name', bucket, prefix || '%');
END;
$$;

--
-- uh_list_objects(bucket, prefix): return all objects in `bucket` with a
-- given `prefix` that are bigger than `lower_bound`
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket regclass, prefix text, lower_bound text)
    RETURNS TABLE(id bigint, name text, size bigint, last_modified TIMESTAMP, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    IF lower_bound = '' THEN
        RETURN QUERY EXECUTE
            format('SELECT id, name, size, last_modified, etag FROM %s WHERE name LIKE %L AND name > %L ORDER BY name',
                bucket, prefix || '%', lower_bound);
    ELSE
        RETURN QUERY EXECUTE
            format('SELECT id, name, size, last_modified, etag FROM %s WHERE name LIKE %L AND name > %L AND NOT starts_with(name, %L) ORDER BY name',
                bucket, prefix || '%', lower_bound, lower_bound);
    END IF;
END;
$$;

--
--
--
--
CREATE OR REPLACE FUNCTION uh_bucket_size(bucket regclass)
    RETURNS BIGINT
LANGUAGE plpgsql AS $$
DECLARE result BIGINT;
BEGIN
    CALL uh_check_bucket(bucket);
    EXECUTE format('SELECT sum(size) FROM %s', bucket) INTO result;
    RETURN result;
END;
$$;
