-- ------------------------------------------------------------------------
--
-- Database tables
--

--
-- The table `uploads` has a single entry for each active multipart upload.
--
CREATE TABLE uploads (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    bucket TEXT NOT NULL,
    key TEXT NOT NULL,
    erased_since TIMESTAMP DEFAULT NULL, -- when not null, record is deleted
    mime TEXT DEFAULT NULL
    );

--
-- `upload_parts` contains information about a single upload part
--
CREATE TABLE upload_parts (
    id bigint GENERATED ALWAYS AS IDENTITY,
    upload_id UUID REFERENCES uploads(id) ON DELETE CASCADE NOT NULL,
    part_id BIGINT NOT NULL,
    size BIGINT NOT NULL,
    effective_size BIGINT NOT NULL,
    address BYTEA NOT NULL,
    etag TEXT NOT NULL,
    UNIQUE (upload_id, part_id));

-- ------------------------------------------------------------------------
--
-- Database functions for controlling the multipart state
--

--
-- uh_create_upload(bucket, key) -- create an empty multipart upload and
-- return its ID
--
CREATE OR REPLACE FUNCTION uh_create_upload(bucket TEXT, key TEXT, mime TEXT) RETURNS TEXT
LANGUAGE plpgsql AS $$
DECLARE id TEXT;
BEGIN
    EXECUTE format('INSERT INTO uploads (bucket, key, mime) VALUES(%L, %L, %L) RETURNING id', bucket, key, mime)
        INTO id;
    RETURN id;
END;
$$;

--
-- uh_put_multipart(id, part_id, size, effective_size, address, etag) --
-- register an uploaded part with a multipart upload
--
CREATE OR REPLACE PROCEDURE uh_put_multipart(id TEXT, part_id BIGINT, size BIGINT, effective_size BIGINT, address BYTEA, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('
        INSERT INTO upload_parts (upload_id, part_id, size, effective_size, address, etag)
        VALUES (%L, %L, %L, %L, %L, %L) ON CONFLICT(upload_id, part_id) DO UPDATE SET
        size = EXCLUDED.size, effective_size = EXCLUDED.effective_size, address = EXCLUDED.address, etag = EXCLUDED.etag',
        id, part_id, size, effective_size, address, etag);
END;
$$;

--
-- uh_get_upload(id) -- return metadata for upload id
--
CREATE OR REPLACE FUNCTION uh_get_upload(id TEXT)
    RETURNS TABLE (bucket TEXT, key TEXT, erased_since TIMESTAMP, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT bucket, key, erased_since, mime FROM uploads WHERE id = %L', id);
END;
$$;

--
-- uh_get_upload_parts(id) -- return list of all upload parts for a given upload
-- id
--
CREATE OR REPLACE FUNCTION uh_get_upload_parts(id TEXT)
    RETURNS TABLE (part_id BIGINT, size BIGINT, effective_size BIGINT, address BYTEA, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT part_id, size, effective_size, address, etag FROM upload_parts WHERE upload_id = %L', id);
END;
$$;

--
-- uh_get_uploads() -- return a list of all active uploads
--
CREATE OR REPLACE FUNCTION uh_get_uploads()
    RETURNS TABLE (id UUID, bucket TEXT, key TEXT, mime TEXT)
LANGUAGE SQL AS 'SELECT id, bucket, key, mime FROM uploads WHERE erased_since IS NULL';

--
-- uh_get_uploads(bucket) -- return a list of all active uploads for the given bucket
--
CREATE OR REPLACE FUNCTION uh_get_uploads(bucket TEXT)
    RETURNS TABLE (id UUID, key TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT id, key, mime FROM uploads WHERE erased_since IS NULL AND bucket = %L', bucket);
END;
$$;

--
-- uh_delete_upload(id) -- delete an upload and all of its parts. This only
-- marks an upload as deleted. Real deletion must be triggered later by
-- calling `uh_clean_deleted`
--
CREATE OR REPLACE PROCEDURE uh_delete_upload(id TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('UPDATE uploads SET erased_since = now(), bucket = gen_random_uuid(), key = gen_random_uuid() WHERE id = %L', id);
END;
$$;

--
-- uh_clean_deleted(age) -- finalize deletion of erased uploads older
-- than the given age.
--
CREATE OR REPLACE PROCEDURE uh_clean_deleted(age INTERVAL)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('DELETE FROM uploads WHERE erased_since + %L < now()', age);
END;
$$;
