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

ALTER TABLE __objects ALTER COLUMN last_modified DROP DEFAULT;

--
-- ceil_timestamp
--
CREATE OR REPLACE FUNCTION ceiled_now()
RETURNS TIMESTAMP
LANGUAGE plpgsql AS $$
BEGIN
    RETURN CURRENT_TIMESTAMP(0) + '1 second'::interval;
END;
$$;


--
-- Re-define uh_put_object
--
CREATE OR REPLACE PROCEDURE uh_put_object(bucket TEXT, object TEXT, address BYTEA, size BIGINT, etag TEXT, mime TEXT)
LANGUAGE plpgsql AS $$
DECLARE bucket_id BIGINT;
BEGIN
    SELECT id INTO bucket_id FROM __buckets WHERE name = bucket;

    IF bucket_id IS NULL THEN
        RAISE EXCEPTION 'Bucket with name % does not exist.', bucket;
    END IF;

    EXECUTE 'UPDATE __objects SET status = status_deleted() WHERE bucket_id = $1 AND name = $2'
        USING bucket_id, object;

    EXECUTE 'INSERT INTO __objects (bucket_id, name, address, size, last_modified, etag, mime)
        VALUES ($1, $2, $3, $4, $5, $6, $7)'
        USING bucket_id, object, address, size, ceiled_now(), etag, mime;
END
$$;
