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

