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
