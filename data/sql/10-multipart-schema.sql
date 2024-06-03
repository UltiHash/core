\c uh_multipart;

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
    UNIQUE (bucket, key));

--
-- `upload_parts` contains information about a single upload part
--
CREATE TABLE upload_parts (
    id bigint GENERATED ALWAYS AS IDENTITY,
    upload_id UUID REFERENCES uploads(id) ON DELETE CASCADE NOT NULL,
    part_id SMALLINT NOT NULL,
    size BIGINT NOT NULL,
    effective_size BIGINT NOT NULL,
    address TEXT NOT NULL,
    etag TEXT NOT NULL,
    UNIQUE (upload_id, part_id));

-- ------------------------------------------------------------------------
--
-- Database functions for controlling the directory
--

--
-- uh_create_upload(bucket, key) -- create an empty multipart upload and
-- return its ID
--
CREATE OR REPLACE FUNCTION uh_create_upload(bucket TEXT, key TEXT) RETURNS TEXT
LANGUAGE plpgsql AS $$
DECLARE id TEXT;
BEGIN
    EXECUTE format('INSERT INTO uploads (bucket, key) VALUES(%L, %L) RETURNING id', bucket, key)
        INTO id;
    RETURN id;
END;
$$;

--
-- uh_put_multipart(id, part_id, size, effective_size, address, etag) --
-- register an uploaded part with a multipart upload
--
CREATE OR REPLACE PROCEDURE uh_put_multipart(id TEXT, part_id SMALLINT, size BIGINT, effective_size BIGINT, address TEXT, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('
        INSERT INTO upload_parts (upload_id, part_id, size, effective_size, address, etag)
        VALUES (%L, %L, %L, %L, %L, %L) ON CONFLICT(upload_id, part_id) DO UPDATE SET
        size = EXCLUDED.size, effective_size = EXCLUDED.effective_size, address = EXCLUDED.address, etag = EXCLUDED.etag',
        id, part_d, size, effective_size, address, etag);
END;
$$;

--
-- uh_get_upload(id) -- return metadata for upload id
--
CREATE OR REPLACE FUNCTION uh_get_upload(id TEXT)
    RETURNS TABLE (bucket TEXT, key TEXT, erased TIMESTAMP)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT bucket, key, erased FROM uploads WHERE id = %L', id);
END;
$$;

--
-- uh_get_upload_parts(id) -- return list of all upload parts for a given upload
-- id
--
CREATE OR REPLACE FUNCTION uh_get_upload_parts(id TEXT)
    RETURNS TABLE (part_id SMALLINT, size BIGINT, effective_size BIGINT, address TEXT, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT part_id, size, effective_size, address, etag FROM upload_parts WHERE id = %L', id);
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
    EXECUTE format('UPDATE uploads SET erased_since = now() WHERE id = %L', id);
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
