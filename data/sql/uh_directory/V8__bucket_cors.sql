--
-- Add column cors to bucket table
--
ALTER TABLE buckets ADD COLUMN cors XML;

--
-- uh_get_bucket(bucket): get policy and cors for a bucket
--
CREATE OR REPLACE FUNCTION uh_bucket_cors(bucket TEXT)
    RETURNS TABLE(cors XML)
LANGUAGE plpgsql AS $$
DECLARE cors_record XML;
BEGIN
    SELECT buckets.cors INTO cors_record FROM buckets WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;

    RETURN QUERY SELECT cors_record;
END;
$$;

--
-- uh_bucket_set_cors(bucket): set the cors for a bucket
--
CREATE OR REPLACE PROCEDURE uh_bucket_set_cors(bucket TEXT, config XML)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE buckets SET cors = XMLPARSE(DOCUMENT config) WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;
END;
$$;

