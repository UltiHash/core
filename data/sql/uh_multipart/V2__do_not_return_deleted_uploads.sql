ALTER TABLE uploads ADD COLUMN complete INT NOT NULL DEFAULT 0;

--
-- uh_complete_upload(id) -- mark an upload as completed
--
CREATE OR REPLACE PROCEDURE uh_complete_upload(id TEXT)
LANGUAGE plpgsql AS $$
BEGIN
    EXECUTE format('UPDATE uploads SET complete = 1 WHERE id = %L', id);
END;
$$;


DROP FUNCTION uh_get_upload(id TEXT);

--
-- uh_get_upload(id) -- return metadata for upload id
--
CREATE OR REPLACE FUNCTION uh_get_upload(id TEXT)
    RETURNS TABLE (bucket TEXT, key TEXT, erased_since TIMESTAMP, mime TEXT, complete INT)
LANGUAGE plpgsql AS $$
BEGIN
    RETURN QUERY EXECUTE format('SELECT bucket, key, erased_since, mime, complete FROM uploads WHERE id = %L', id);
END;
$$;

