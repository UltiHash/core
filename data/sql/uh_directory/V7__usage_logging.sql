-- ------------------------------------------------------------------------
--
-- Database tables
--

-- The table "object_refs" only contains entries for objects with one or
-- multiple downloads in progress
--
CREATE TABLE object_refs (
    object_id   BIGINT PRIMARY KEY REFERENCES __objects ON DELETE RESTRICT,
    refs        INTEGER NOT NULL
);

CREATE TABLE object_status (
    object_id   BIGINT PRIMARY KEY REFERENCES __objects ON DELETE RESTRICT,
    status      INTEGER NOT NULL,
    deleted_at  TIMESTAMP DEFAULT now() NOT NULL
);

CREATE TABLE bucket_status (
    bucket_id   BIGINT PRIMARY KEY REFERENCES __buckets ON DELETE RESTRICT,
    status      INTEGER NOT NULL,
    deleted_at  TIMESTAMP DEFAULT now() NOT NULL
);

-- Migrate refs data from __objects table into object_refs table
DO $$
BEGIN
    EXECUTE format(
        'INSERT INTO object_refs (object_id, refs)
         SELECT id, refs FROM __objects WHERE refs > 0'
    );
END $$;

-- Migrate status data from __objects table into object_status table
DO $$
BEGIN
EXECUTE format(
        'INSERT INTO object_status (object_id, status)
         SELECT id, status FROM __objects WHERE status > 0'
        );
END $$;

-- Migrate status data from __buckets table into bucket_status table
DO $$
BEGIN
    EXECUTE format(
        'INSERT INTO bucket_status (bucket_id, status)
         SELECT id, status FROM __buckets WHERE status > 0'
    );
END $$;

ALTER TABLE __objects
    DROP COLUMN refs,
    DROP COLUMN status;

ALTER TABLE __objects
    RENAME TO objects;

ALTER TABLE __buckets
    ADD COLUMN last_modified TIMESTAMP DEFAULT now() NOT NULL,
    ADD COLUMN version uuid DEFAULT gen_random_uuid() NOT NULL,
    DROP COLUMN status;

ALTER TABLE __buckets
    RENAME TO buckets;



CREATE OR REPLACE FUNCTION uh_get_bucket_id(bucket TEXT)
    RETURNS BIGINT
    LANGUAGE plpgsql AS $$
DECLARE b_id BIGINT;
BEGIN
    SELECT b.id INTO b_id
    FROM buckets b
        LEFT JOIN bucket_status s ON b.id = s.bucket_id
    WHERE b.name = bucket
        AND s.bucket_id IS NULL
    ORDER BY b.id DESC
    LIMIT 1;

    IF b_id IS NULL THEN
        RAISE EXCEPTION 'Bucket "%" does not exist.', bucket;
    END IF;

    RETURN b_id;
END;
$$;

---
--- re-define uh_bucket_policy()
---
CREATE OR REPLACE FUNCTION uh_bucket_policy(bucket TEXT)
    RETURNS TABLE(policy JSON)
    LANGUAGE plpgsql AS $$
DECLARE policy_record JSON;
BEGIN
    -- Execute the query and save the result to a variable
    SELECT buckets.policy INTO policy_record
    FROM buckets
    WHERE name = bucket;

    -- Check if the bucket was found
    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;

    -- Return the policy
    RETURN QUERY SELECT policy_record;
END;
$$;

---
--- re-define uh_bucket_set_policy()
---
CREATE OR REPLACE PROCEDURE uh_bucket_set_policy(bucket TEXT, new_policy JSON)
    LANGUAGE plpgsql AS $$
BEGIN
    UPDATE buckets
    SET policy = new_policy
    WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;
END;
$$;


--
-- re-define uh_check_bucket()
--
CREATE OR REPLACE PROCEDURE uh_check_bucket(bucket TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    PERFORM 1
    FROM buckets b
             LEFT JOIN bucket_status s ON b.id = s.bucket_id
    WHERE b.name = bucket
      AND s.bucket_id IS NULL
    ORDER BY b.id DESC
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
    DELETE FROM buckets b
    USING bucket_status s
    WHERE b.id = s.bucket_id
      AND s.status = status_deleted();
END
$$;

--
-- re-define uh_create_bucket()
--
CREATE OR REPLACE PROCEDURE uh_create_bucket(bucket TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    INSERT INTO buckets (name) VALUES (bucket);
END
$$;


--
-- re-define uh_data_size()
--
CREATE OR REPLACE FUNCTION uh_data_size() RETURNS BIGINT
    LANGUAGE plpgsql AS $$
DECLARE result BIGINT;
BEGIN
    SELECT SUM(size) INTO result
    FROM objects o
         LEFT JOIN object_status s ON o.id = s.object_id
    WHERE s.object_id IS NULL;
    RETURN COALESCE(result, 0);
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

    INSERT INTO bucket_status (bucket_id, status)
    VALUES (b_id, status_deleted())
    ON CONFLICT DO NOTHING;
END;
$$;


--
-- re-define uh_delete_object()
--
CREATE OR REPLACE PROCEDURE uh_delete_object(bucket TEXT, object TEXT)
    LANGUAGE plpgsql AS $$
DECLARE
    obj_id BIGINT;
BEGIN
    SELECT o.id INTO obj_id
    FROM objects o
        LEFT JOIN object_status s ON o.id = s.object_id
    WHERE o.bucket_id = uh_get_bucket_id(bucket)
      AND o.name = object
      AND s.object_id IS NULL
    ORDER BY o.id DESC
    LIMIT 1;

    IF obj_id IS NULL THEN
        RAISE EXCEPTION 'Cannot delete object "%" in bucket "%", as it does not appear to exist.', object, bucket;
    END IF;

    INSERT INTO object_status (object_id, status)
    VALUES (obj_id, status_deleted())
    ON CONFLICT DO NOTHING;
END;
$$;

--
-- Re-define uh_delete_object_by_id()
--
CREATE OR REPLACE PROCEDURE uh_delete_object_by_id(target_id BIGINT)
    LANGUAGE plpgsql AS $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM object_status s
                 LEFT JOIN object_refs r ON s.object_id = r.object_id
        WHERE s.object_id = target_id
          AND s.status = status_collected()
          AND r.object_id IS NULL
    ) THEN
        RAISE EXCEPTION 'Object cannot be deleted: it is either not collected or has active references.';
    END IF;

    DELETE FROM object_status WHERE object_id = target_id;
    DELETE FROM objects WHERE id = target_id;
END
$$;

--
-- Re-define uh_get_object()
--
CREATE OR REPLACE FUNCTION uh_get_object(bucket TEXT, object TEXT)
    RETURNS TABLE (id BIGINT, address BYTEA, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY
        SELECT o.id, o.address, o.size, o.last_modified, o.etag, o.mime
         FROM objects o
         LEFT JOIN object_status s ON o.id = s.object_id
         WHERE o.bucket_id = uh_get_bucket_id(bucket)
           AND o.name = object
           AND s.object_id IS NULL
        ORDER BY o.id DESC
        LIMIT 1;
END;
$$;

--
--  Re-define uh_inc_reference()
--
CREATE OR REPLACE PROCEDURE uh_inc_reference(target_id BIGINT)
    LANGUAGE plpgsql AS $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM objects o
        WHERE o.id = target_id
    ) THEN
        RAISE EXCEPTION 'object not found';
    END IF;

    INSERT INTO object_refs (object_id, refs)
    VALUES (target_id, 1)
    ON CONFLICT (object_id)
        DO UPDATE SET refs = object_refs.refs + 1;
END
$$;

--
-- Re-define uh_dec_reference()
--
CREATE OR REPLACE PROCEDURE uh_dec_reference(target_id BIGINT)
    LANGUAGE plpgsql AS $$
BEGIN
    IF NOT EXISTS (
        SELECT 1
        FROM objects o
        WHERE o.id = target_id
    ) THEN
        RAISE EXCEPTION 'object not found';
    END IF;

    UPDATE object_refs
    SET refs = refs - 1
    WHERE object_id = target_id;

    DELETE FROM object_refs
    WHERE object_id = target_id AND refs <= 0;
END
$$;

--
-- Re-define uh_list_buckets()
--
DROP FUNCTION uh_list_buckets();
CREATE OR REPLACE FUNCTION uh_list_buckets()
    RETURNS TABLE(name VARCHAR(64))
    LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY
        SELECT b.name
        FROM buckets b
            LEFT JOIN bucket_status s ON b.id = s.bucket_id
        WHERE s.bucket_id IS NULL;
END;
$$;

--
-- Re-define uh_list_objects(bucket)
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
        FROM objects o
            LEFT JOIN object_status s ON o.id = s.object_id
        WHERE o.bucket_id = (SELECT b.id FROM buckets b WHERE b.name = bucket)
          AND s.object_id IS NULL
        ORDER BY o.name;
END;
$$;

--
-- Re-define uh_list_objects(bucket, prefix)
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT, prefix TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
        FROM objects o
            JOIN buckets b ON o.bucket_id = b.id
            LEFT JOIN object_status s ON o.id = s.object_id
        WHERE b.name = bucket
          AND o.name LIKE prefix || '%'
          AND s.object_id IS NULL
        ORDER BY o.name;
END;
$$;

--
-- Re-define uh_list_objects(bucket, prefix, lower_bound)
--
CREATE OR REPLACE FUNCTION uh_list_objects(bucket TEXT, prefix TEXT, lower_bound TEXT)
    RETURNS TABLE(id BIGINT, name TEXT, size BIGINT, last_modified TIMESTAMP, etag TEXT, mime TEXT)
    LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);
    RETURN QUERY
        SELECT o.id, o.name, o.size, o.last_modified, o.etag, o.mime
        FROM objects o
            JOIN buckets b ON o.bucket_id = b.id
            LEFT JOIN object_status s ON o.id = s.object_id
        WHERE b.name = bucket
          AND o.name LIKE prefix || '%'
          AND s.object_id IS NULL
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
       FROM object_status s
          INNER JOIN objects o on o.id = s.object_id
          LEFT JOIN object_refs r on s.object_id = r.object_id
       WHERE s.status = status_deleted() AND r.object_id IS NULL
       LIMIT 1;

    IF NOT FOUND THEN
       RETURN;
    END IF;

    UPDATE object_status
    SET status = status_collected()
    WHERE o.id = target_id;

    RETURN QUERY SELECT target_id, target_address;
END
$$;

--
-- Re-define uh_put_object
--
CREATE OR REPLACE PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT)
    LANGUAGE plpgsql AS $$
DECLARE b_id BIGINT;
        o_id BIGINT;
BEGIN
    SELECT uh_get_bucket_id(bucket) INTO b_id;

    SELECT id INTO o_id FROM uh_get_object(bucket, object);

    IF o_id IS NOT NULL THEN
       INSERT INTO object_status (object_id, status)
       VALUES (o_id, status_deleted())
       ON CONFLICT DO NOTHING;
    END IF;

    EXECUTE 'INSERT INTO objects (bucket_id, name, address, size, last_modified, etag, mime)
            VALUES ($1, $2, $3, $4, $5, $6, $7)'
       USING b_id, object, address, size, ceiled_now(), etag, mime;
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
        SELECT o.size, o.last_modified, s.deleted_at
        FROM objects o
                 LEFT JOIN object_status s ON o.id = s.object_id
        WHERE o.last_modified < interval_end
          AND (s.status IS NULL OR s.deleted_at >= interval_start)
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
