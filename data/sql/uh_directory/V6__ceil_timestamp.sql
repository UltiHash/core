ALTER TABLE __objects ALTER COLUMN last_modified DROP DEFAULT;

--
-- ceil_timestamp
--
CREATE OR REPLACE FUNCTION ceiled_now()
RETURNS TIMESTAMP
LANGUAGE plpgsql AS $$
BEGIN
    RETURN CURRENT_TIMESTAMP(0) +
        CASE WHEN CURRENT_TIMESTAMP != CURRENT_TIMESTAMP(0)
            THEN '1 second'::interval ELSE '0 second'::interval END;
END;
$$;


--
-- Re-define uh_put_object
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

    EXECUTE 'INSERT INTO __objects (bucket_id, name, address, size, last_modified, etag, mime)
        VALUES ($1, $2, $3, $4, $5, $6, $7)'
        USING bucket_id, object, address, size, ceiled_now(), etag, mime;
END
$$;
