--
-- Add column policy to __bucket table
--
ALTER TABLE __buckets ADD COLUMN policy JSON;

--
-- uh_bucket_policy(bucket): get the policy from a bucket
--
CREATE OR REPLACE FUNCTION uh_bucket_policy(bucket regclass)
    RETURNS TABLE(policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    RETURN QUERY EXECUTE format('SELECT policy FROM __buckets WHERE name = %L', rel_name(bucket));
END;
$$;

--
-- uh_bucket_set_policy(bucket): set the policy for a bucket
--
CREATE OR REPLACE PROCEDURE uh_bucket_set_policy(bucket regclass, policy JSON)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE format('UPDATE __buckets SET policy = %L WHERE name = %L', policy, rel_name(bucket));
END;
$$;

