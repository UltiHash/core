-- remove unused functions
DROP FUNCTION IF EXISTS rel_name;
DROP PROCEDURE IF EXISTS uh_lock_object;
DROP PROCEDURE IF EXISTS uh_unlock_object;
DROP PROCEDURE IF EXISTS uh_lock_object_shared;
DROP PROCEDURE IF EXISTS uh_unlock_object_shared;
DROP FUNCTION IF EXISTS uh_bucket_size;


-- Merge object_status with objects
ALTER TABLE object_status RENAME TO former_object_status;
CREATE TYPE object_status as ENUM ('Normal', 'Deleted', 'Collected');

ALTER TABLE objects ADD COLUMN
    status object_status NOT NULL DEFAULT 'Normal';
ALTER TABLE objects ADD COLUMN
    deleted_at TIMESTAMP DEFAULT NULL;
ALTER TABLE objects ADD COLUMN
    sticky BOOLEAN NOT NULL DEFAULT False;

CREATE OR REPLACE FUNCTION migrate_object_status(id bigint)
    RETURNS object_status
LANGUAGE plpgsql AS $$
DECLARE status INTEGER;
BEGIN
    SELECT status FROM former_object_status INTO status WHERE object_id = id;

    IF status = status_normal() THEN
        RETURN 'Normal';
    END IF;

    IF status = status_deleted() THEN
        RETURN 'Deleted';
    END IF;

    IF status = status_collected() THEN
        RETURN 'Collected';
    END IF;

    RETURN 'Normal';
END;
$$;

UPDATE objects SET status = migrate_object_status(id);
UPDATE objects SET deleted_at = former_object_status.deleted_at
    FROM former_object_status WHERE id = object_id;
DROP TABLE former_object_status;
DROP FUNCTION IF EXISTS migrate_object_state;

-- Merge bucket_status with buckets
ALTER TABLE bucket_status RENAME TO former_bucket_status;
CREATE TYPE bucket_status as ENUM ('Normal', 'Deleted');

ALTER TABLE buckets ADD COLUMN
    status bucket_status NOT NULL DEFAULT 'Normal';
ALTER TABLE buckets ADD COLUMN
    deleted_at TIMESTAMP DEFAULT NULL;

CREATE OR REPLACE FUNCTION migrate_bucket_status(id bigint)
    RETURNS bucket_status
LANGUAGE plpgsql AS $$
DECLARE status INTEGER;
BEGIN
    SELECT status FROM former_bucket_status INTO status WHERE bucket_id = id;

    IF status = status_normal() THEN
        RETURN 'Normal';
    END IF;

    IF status = status_deleted() THEN
        RETURN 'Deleted';
    END IF;

    RETURN 'Normal';
END;
$$;

UPDATE buckets SET status = migrate_bucket_status(id);
UPDATE buckets SET deleted_at = former_bucket_status.deleted_at
    FROM former_bucket_status WHERE id = bucket_id;
DROP TABLE former_bucket_status;
DROP FUNCTION IF EXISTS migrate_bucket_status;


CREATE OR REPLACE FUNCTION uh_get_bucket_id(bucket TEXT)
    RETURNS BIGINT
    LANGUAGE plpgsql AS $$
DECLARE b_id BIGINT;
BEGIN
    SELECT id INTO b_id FROM buckets WHERE status != 'Deleted' AND name = bucket
    ORDER BY id DESC
    LIMIT 1;

    IF b_id IS NULL THEN
        RAISE EXCEPTION 'Bucket "%" does not exist.', bucket;
    END IF;

    RETURN b_id;
END;
$$;

CREATE OR REPLACE FUNCTION uh_get_bucket_info(bucket TEXT)
    RETURNS TABLE(id BIGINT, versioning versioning_type)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY SELECT buckets.id, buckets.versioning FROM buckets WHERE name = bucket AND status = 'Normal';
END;
$$;

--
-- re-define uh_delete_bucket()
--
CREATE OR REPLACE PROCEDURE uh_delete_bucket(bucket TEXT)
    LANGUAGE plpgsql AS $$
DECLARE
    b_id BIGINT;
BEGIN
    SELECT uh_get_bucket_id(bucket) INTO b_id;
    UPDATE buckets SET status = 'Deleted' WHERE id = b_id;
END;
$$;

--
-- re-define uh_check_bucket()
--
CREATE OR REPLACE PROCEDURE uh_check_bucket(bucket TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    PERFORM 1 FROM buckets WHERE name = bucket AND status = 'Normal'
    ORDER BY id DESC
    LIMIT 1;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;
END
$$;

--
-- re-define uh_clear_deleted_buckets()
--
CREATE OR REPLACE PROCEDURE uh_clear_deleted_buckets()
    LANGUAGE plpgsql AS $$
BEGIN
    DELETE FROM buckets b WHERE status = 'Deleted'
        AND NOT EXISTS (SELECT 1 FROM objects o WHERE o.bucket_id = b.id);
END
$$;

--
-- Re-define uh_list_buckets()
--
CREATE OR REPLACE FUNCTION uh_list_buckets()
    RETURNS TABLE(name VARCHAR(64))
    LANGUAGE sql AS 'SELECT name FROM buckets WHERE status = ''Normal'';';

--
-- re-define uh_create_bucket()
--
CREATE OR REPLACE PROCEDURE uh_create_bucket(bucket TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    PERFORM 1 FROM buckets WHERE name = bucket AND status = 'Normal';

    IF FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" already exists', bucket;
    END IF;

    INSERT INTO buckets (name) VALUES (bucket);
END
$$;

--
-- Re-define uh_data_size()
--
CREATE OR REPLACE FUNCTION uh_data_size() RETURNS BIGINT
    LANGUAGE plpgsql AS $$
DECLARE result BIGINT;
BEGIN
    SELECT SUM(size) INTO result FROM objects WHERE status = 'Normal';
    RETURN COALESCE(result, 0);
END;
$$;

--
-- Re-define uh_delete_object()
--
DROP PROCEDURE IF EXISTS uh_delete_object;
CREATE OR REPLACE FUNCTION uh_delete_object(bucket TEXT, object TEXT)
    RETURNS TABLE (delete_marker BOOLEAN, version UUID)
    LANGUAGE plpgsql AS $$
DECLARE
    b_versioning versioning_type;
    obj_id BIGINT;
    obj_sticky BOOLEAN;
    obj_status object_status;
BEGIN
    SELECT versioning FROM uh_get_bucket_info(bucket) INTO b_versioning;
    SELECT id, sticky, status FROM uh_get_object(bucket, object) INTO obj_id, obj_sticky, obj_status;

    IF obj_id IS NULL OR obj_status = 'Deleted' THEN
        RAISE EXCEPTION 'Cannot delete object "%" in bucket "%", as it does not appear to exist.', object, bucket;
    END IF;

    IF b_versioning = 'Enabled' THEN

        SELECT id FROM uh_put_object(bucket, object, '', 0, NULL, NULL) INTO obj_id;
        UPDATE objects SET status = 'Deleted', deleted_at = now(), sticky = True WHERE id = obj_id;

        RETURN QUERY SELECT True, o.version FROM objects o WHERE id = obj_id;

    ELSEIF b_versioning = 'Suspended' THEN

        IF obj_sticky THEN
            SELECT id FROM uh_put_object(bucket, object, '', 0, NULL, NULL) INTO obj_id;
        END IF;

        UPDATE objects SET status = 'Deleted', deleted_at = now(), sticky = True WHERE id = obj_id;

        RETURN QUERY SELECT True, o.version FROM objects o WHERE id = obj_id;

    ELSE

        UPDATE objects SET status = 'Deleted', deleted_at = now() WHERE id = obj_id;
        RETURN QUERY SELECT True, NULL::UUID FROM objects WHERE id = obj_id;

    END IF;
END;
$$;

--
-- Define uh_delete_object_version()
--
CREATE OR REPLACE FUNCTION uh_delete_object_version(bucket TEXT, object TEXT, version_id UUID)
    RETURNS TABLE (delete_marker BOOLEAN, version UUID)
    LANGUAGE plpgsql AS $$
DECLARE
    obj_id BIGINT;
    obj_sticky BOOLEAN;
    obj_status object_status;
BEGIN
    SELECT id, sticky, status FROM uh_get_object_by_version(bucket, object, version_id) INTO obj_id, obj_sticky, obj_status;

    IF obj_id IS NULL THEN
        RAISE EXCEPTION 'Cannot delete object "%" in bucket "%", as it does not appear to exist.', object, bucket;
    END IF;

    UPDATE objects SET status = 'Deleted', deleted_at = now(), sticky = False WHERE id = obj_id;
    RETURN QUERY SELECT obj_status = 'Deleted', NULL::UUID;
END;
$$;

--
-- Define uh_delete_object_null_version()
--
CREATE OR REPLACE FUNCTION uh_delete_object_null_version(bucket TEXT, object TEXT)
    RETURNS TABLE (delete_marker BOOLEAN, version UUID)
    LANGUAGE plpgsql AS $$
DECLARE
    obj_id BIGINT;
BEGIN
    SELECT id FROM objects WHERE NOT sticky AND status = 'Normal' AND name = object AND bucket_id = uh_get_bucket_id(bucket) INTO obj_id;

    IF obj_id IS NULL THEN
        RAISE EXCEPTION 'Cannot delete object "%" in bucket "%", as it does not appear to exist.', object, bucket;
    END IF;

    UPDATE objects SET status = 'Deleted', deleted_at = now() WHERE id = obj_id;
    RETURN QUERY SELECT False, NULL::UUID;
END;
$$;

--
-- Re-define uh_delete_object_by_id()
--
CREATE OR REPLACE PROCEDURE uh_delete_object_by_id(target_id BIGINT)
    LANGUAGE plpgsql AS $$
BEGIN
    DELETE FROM objects WHERE id = target_id;
END
$$;

--
-- Re-define uh_get_object()
--
DROP FUNCTION uh_get_object(bucket TEXT, object TEXT);
CREATE OR REPLACE FUNCTION uh_get_object(bucket TEXT, object TEXT)
    RETURNS TABLE (id BIGINT, name TEXT, address BYTEA, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT, version UUID, status object_status, sticky BOOLEAN)
    LANGUAGE plpgsql AS $$
DECLARE
    b_id BIGINT;
    b_ver versioning_type;
BEGIN
    SELECT uh_get_bucket_info.id, versioning FROM uh_get_bucket_info(bucket) INTO b_id, b_ver;

    IF b_ver = 'Disabled' THEN

        RETURN QUERY
            SELECT o.id, o.name, o.address, o.size, o.last_modified, o.etag, o.mime, NULL::UUID, o.status, o.sticky FROM objects o
            JOIN (
                SELECT max(o2.id) AS max_id FROM objects o2 WHERE o2.status = 'Normal' AND o2.bucket_id = uh_get_bucket_id(bucket) AND o2.name = object
            ) temp ON o.id = temp.max_id;

    ELSE

        RETURN QUERY
            SELECT o.id, o.name, o.address, o.size, o.last_modified, o.etag, o.mime, o.version, o.status, o.sticky FROM objects o
            JOIN (
                SELECT max(o2.id) AS max_id FROM objects o2 WHERE NOT (o2.status = 'Deleted' and NOT o2.sticky) AND o2.bucket_id = uh_get_bucket_id(bucket) AND o2.name = object
            ) temp ON o.id = temp.max_id;
    END IF;
END;
$$;

--
-- define uh_get_object_by_version()
--
CREATE OR REPLACE FUNCTION uh_get_object_by_version(bucket TEXT, object TEXT, ver UUID)
    RETURNS TABLE (id BIGINT, name TEXT, address BYTEA, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT, version UUID, status object_status, sticky BOOLEAN)
    LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY
        SELECT o.id, o.name, o.address, o.size, o.last_modified, o.etag, o.mime, o.version, o.status, o.sticky
         FROM objects o
         WHERE bucket_id = uh_get_bucket_id(bucket) AND o.name = object AND o.version = ver
        ORDER BY id DESC
        LIMIT 1;
END;
$$;

--
-- Re-define uh_list_objects(bucket)
--
DROP FUNCTION uh_list_objects(bucket TEXT);
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT, version UUID, status object_status)
    LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime, o.version, o.status FROM objects o
        JOIN (
            SELECT o2.name, o2.bucket_id, max(o2.id) AS max_id FROM objects o2 GROUP BY o2.name, o2.bucket_id
        ) temp ON o.id = temp.max_id
        WHERE o.bucket_id = uh_get_bucket_id(bucket) AND o.status = 'Normal'
        ORDER BY o.name;
END;
$$;

--
-- Re-define uh_list_objects(bucket, prefix)
--
DROP FUNCTION uh_list_objects(bucket TEXT, prefix TEXT);
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT, prefix TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT, version UUID, status object_status)
    LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime, o.version, o.status FROM objects o
        JOIN (
            SELECT o2.name, o2.bucket_id, max(o2.id) AS max_id FROM objects o2 GROUP BY o2.name, o2.bucket_id
        ) temp ON o.id = temp.max_id
        WHERE o.bucket_id = uh_get_bucket_id(bucket)
          AND o.name LIKE prefix || '%'
          AND o.status = 'Normal'
        ORDER BY o.name;
END;
$$;

--
-- Re-define uh_list_objects(bucket, prefix, lower_bound)
--
DROP FUNCTION uh_list_objects(bucket TEXT, prefix TEXT, lower_bound TEXT);
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT, prefix TEXT, lower_bound TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT, version UUID, status object_status)
    LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime, o.version, o.status FROM objects o
        JOIN (
            SELECT o2.name, o2.bucket_id, max(o2.id) AS max_id FROM objects o2 GROUP BY o2.name, o2.bucket_id
        ) temp ON o.id = temp.max_id
        WHERE o.bucket_id = uh_get_bucket_id(bucket)
          AND o.name LIKE prefix || '%'
          AND o.status = 'Normal'
          AND (lower_bound = '' OR
               (o.name > lower_bound AND NOT starts_with(o.name, lower_bound)))
        ORDER BY o.name;
END;
$$;

--
-- Re-define uh_next_deleted
--
CREATE OR REPLACE FUNCTION uh_next_deleted()
    RETURNS TABLE(id BIGINT, address BYTEA)
    LANGUAGE plpgsql AS $$
DECLARE target_id BIGINT;
        target_address BYTEA;
BEGIN
    SELECT o.id, o.address
       INTO target_id, target_address
    FROM objects o
    LEFT JOIN object_refs r ON o.id = r.object_id
    LEFT JOIN buckets b ON b.id = o.bucket_id
    WHERE o.status = 'Deleted' AND o.sticky = False AND r.object_id IS NULL
    LIMIT 1;

    IF NOT FOUND THEN
       RETURN;
    END IF;

    UPDATE objects o SET status = 'Collected' WHERE o.id = target_id;

    RETURN QUERY SELECT target_id, target_address;
END
$$;

--
-- Re-define uh_put_object
--
DROP PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT);
CREATE OR REPLACE FUNCTION uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT)
    RETURNS TABLE(id BIGINT, version UUID)
    LANGUAGE plpgsql AS $$
DECLARE b_id BIGINT;
        b_ver versioning_type;
        o_id BIGINT;
        o_sticky BOOLEAN;
BEGIN
    SELECT uh_get_bucket_info.id, versioning FROM uh_get_bucket_info(bucket) INTO b_id, b_ver;

    IF b_ver = 'Disabled' THEN

        SELECT uh_get_object.id FROM uh_get_object(bucket, object) INTO o_id;

        IF o_id IS NOT NULL THEN
            UPDATE objects SET status = 'Deleted', deleted_at = now() WHERE objects.id = o_id;
        END IF;

        RETURN QUERY INSERT INTO objects (bucket_id, name, address, size, last_modified, etag, mime)
            VALUES (b_id, object, address, size, ceiled_now(), etag, mime) RETURNING objects.id, NULL::UUID;

    ELSEIF b_ver = 'Enabled' THEN

        RETURN QUERY INSERT INTO objects (bucket_id, name, address, size, last_modified, etag, mime, sticky)
            VALUES (b_id, object, address, size, ceiled_now(), etag, mime, True) RETURNING objects.id, objects.version;

    ELSEIF b_ver = 'Suspended' THEN

        SELECT uh_get_object.id, sticky FROM uh_get_object(bucket, object) INTO o_id, o_sticky;

        IF o_id IS NOT NULL AND NOT o_sticky THEN
            UPDATE objects SET status = 'Deleted', deleted_at = now() WHERE objects.id = o_id;
        END IF;

        RETURN QUERY INSERT INTO objects (bucket_id, name, address, size, last_modified, etag, mime)
            VALUES (b_id, object, address, size, ceiled_now(), etag, mime) RETURNING objects.id, NULL::UUID;

    END IF;
END
$$;

CREATE OR REPLACE FUNCTION uh_list_object_versions(bucket TEXT, prefix TEXT, key_marker TEXT, version_marker UUID, max_keys INTEGER)
    RETURNS TABLE (id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT, version UUID, status object_status)
    LANGUAGE plpgsql AS $$
DECLARE
    min_id BIGINT;
    condition TEXT;
    query TEXT;
BEGIN
    IF version_marker IS NOT NULL THEN
        SELECT o.id FROM objects o, buckets b WHERE o.bucket_id = b.id AND b.name = bucket AND o.name = key_marker AND o.version = version_marker INTO min_id;

        IF FOUND THEN
            SELECT format(' AND o.id >= %s', min_id) INTO condition;
        END IF;
    END IF;

    IF prefix IS NOT NULL THEN
        SELECT format('%s AND o.name LIKE %L || ''%%''', condition, prefix) INTO condition;
    END IF;

    RETURN QUERY EXECUTE format('SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime, o.version, o.status FROM objects o, buckets b WHERE '
        || ' o.bucket_id = b.id AND b.name = %L AND (o.status = ''Normal'' OR o.sticky) %s ORDER BY o.name ASC, o.id DESC LIMIT %L' , bucket, condition, max_keys);
END
$$;

CREATE OR REPLACE FUNCTION uh_compute_usage(interval_start TIMESTAMP, interval_end TIMESTAMP)
    RETURNS BIGINT
    LANGUAGE plpgsql AS $$
DECLARE
    row RECORD;
    byteseconds BIGINT;
    interval_seconds BIGINT;
BEGIN
    byteseconds := 0;
    FOR row in
        SELECT size, last_modified, deleted_at FROM objects
        WHERE last_modified < interval_end AND (status = 'Normal' OR deleted_at >= interval_start)
        LOOP
            interval_seconds := EXTRACT(EPOCH FROM LEAST(COALESCE(row.deleted_at, interval_end), interval_end) - GREATEST(row.last_modified, interval_start));

            IF interval_seconds < 0 THEN
                RAISE NOTICE 'Interval seconds is negative: %, interval_start: %, interval_end: %, last_modified: %, deleted_at: %', interval_seconds, interval_start, interval_end, row.last_modified, row.deleted_at;
                CONTINUE;
            END IF;

            byteseconds := byteseconds + interval_seconds * row.size;
        END LOOP;
    byteseconds := byteseconds / 1024 / 1024 / 1024;
    RETURN byteseconds;
END;
$$;
