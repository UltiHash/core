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
DROP PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT);

CREATE OR REPLACE PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
DECLARE 
    bucket_id BIGINT;
    current_time TIMESTAMP;
BEGIN
    SELECT id INTO bucket_id FROM __buckets WHERE name = bucket;

    IF bucket_id IS NULL THEN
        RAISE EXCEPTION 'Bucket with name % does not exist.', bucket;
    END IF;

    EXECUTE 
        'INSERT INTO __objects (bucket_id, name, address, size, last_modified, etag, mime)
         VALUES ($1, $2, $3, $4, $5, $6, $7)
         ON CONFLICT (bucket_id, name) DO UPDATE
         SET address = EXCLUDED.address, 
             size = EXCLUDED.size, 
             last_modified = EXCLUDED.last_modified,
             etag = EXCLUDED.etag, 
             mime = EXCLUDED.mime'
    USING bucket_id, object, address, size, ceiled_now(), etag, mime;

END
$$;

--
-- Remove uh_copy_object
--
DROP PROCEDURE uh_copy_object(bucket_src TEXT, object_src TEXT, bucket_dst TEXT, object_dst TEXT);
