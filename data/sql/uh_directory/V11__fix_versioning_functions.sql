--
-- uh_bucket_versioning(bucket): return configured versioning
-- for the given bucket
--
DROP PROCEDURE uh_bucket_set_versioning;
CREATE OR REPLACE PROCEDURE uh_bucket_set_versioning(bucket TEXT, vt versioning_type)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE buckets SET versioning = vt WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;
END;
$$;
