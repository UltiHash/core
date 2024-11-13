--
-- uh_get_upload_part(id, part_id) -- return list of all upload parts for a given upload
-- id
--
CREATE OR REPLACE FUNCTION uh_get_upload_part(upload_id TEXT, part_id BIGINT)
    RETURNS TABLE (size BIGINT, address BYTEA, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
RETURN QUERY EXECUTE format('SELECT size, address, etag FROM upload_parts WHERE upload_id = %L AND part_id = %L', upload_id, part_id);
END;
$$;