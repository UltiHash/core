-- Copyright 2026 UltiHash Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

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

