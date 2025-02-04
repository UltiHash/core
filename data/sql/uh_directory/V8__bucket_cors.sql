--
-- Add column cors to bucket table
--
ALTER TABLE buckets ADD COLUMN cors XML;

--
-- uh_get_bucket(bucket): get policy and cors for a bucket
--
CREATE OR REPLACE FUNCTION uh_get_bucket(bucket TEXT)
    RETURNS TABLE(policy JSON, cors XML)
LANGUAGE plpgsql AS $$
DECLARE policy_record JSON;
DECLARE cors_record XML;
BEGIN
    SELECT policy, cors INTO policy_record, cors_record
    FROM buckets
    WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;

    RETURN QUERY SELECT policy_record, XMLSERIALIZE(DOCUMENT cors_record AS text);
END;
$$;

--
-- uh_bucket_set_cors(bucket): set the cors for a bucket
--
CREATE OR REPLACE PROCEDURE uh_bucket_set_cors(bucket regclass, cors XML)
LANGUAGE plpgsql AS $$
BEGIN
    UPDATE buckets
    SET cors = XMLPARSE(DOCUMENT cors)
    WHERE name = bucket;

    IF NOT FOUND THEN
        RAISE EXCEPTION 'Bucket "%s" does not exist in buckets table', bucket;
    END IF;
END;
$$;

