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
-- uh_get_upload_part(id, part_id) -- return list of all upload parts for a given upload
-- id
--
CREATE OR REPLACE FUNCTION uh_get_upload_part(upload_id TEXT, part_id BIGINT)
    RETURNS TABLE (size BIGINT, address BYTEA, etag TEXT)
LANGUAGE plpgsql AS $$
BEGIN
RETURN QUERY EXECUTE format('SELECT size, address, etag FROM upload_parts WHERE upload_id = %L AND part_id = %L', upload_id, part_id);
END;
$$;