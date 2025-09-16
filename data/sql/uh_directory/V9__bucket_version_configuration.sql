--
-- Enum to describe versioning state
--
CREATE TYPE versioning_type as ENUM ('Disabled', 'Enabled', 'Suspended');

--
-- Add column defining configured versioning for a bucket.
--
ALTER TABLE buckets ADD COLUMN
    versioning versioning_type NOT NULL DEFAULT 'Disabled';

--
-- uh_bucket_versioning(bucket): return configured versioning
-- for the given bucket
--
CREATE OR REPLACE PROCEDURE uh_bucket_set_versioning(bucket TEXT, status versioning_type)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE buckets SET versioning = status WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;
END;
$$;

--
-- uh_bucket_versioning(bucket): return configured versioning
-- for the given bucket
--
CREATE OR REPLACE FUNCTION uh_bucket_versioning(bucket TEXT)
    RETURNS TABLE(status TEXT)
LANGUAGE plpgsql AS $$
DECLARE rv TEXT;
BEGIN
    SELECT versioning INTO rv FROM buckets WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;

    RETURN QUERY SELECT rv;
END;
$$;
