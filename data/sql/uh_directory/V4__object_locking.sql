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
-- Creates a lock on the specified object until `un_unlock_object` is called or
-- the database session is closed. The lock is an advisory lock on a hash of the
-- strings `bucket` and `key`.
--
CREATE OR REPLACE PROCEDURE uh_lock_object(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_lock(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;

--
-- Releases a previously obtained lock for the specified object.
--
CREATE OR REPLACE PROCEDURE uh_unlock_object(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_unlock(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;

--
-- Creates a shared lock on the specified object until `un_unlock_object_shared`
-- is called or the database session is close. The lock is a shared advisory
-- lock on a hash of the strings `bucket` and `key`.
--
CREATE OR REPLACE PROCEDURE uh_lock_object_shared(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_lock_shared(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;

--
-- Releases a previously obtained shared lock for the specified object.
--
CREATE OR REPLACE PROCEDURE uh_unlock_object_shared(bucket regclass, key text)
LANGUAGE plpgsql AS $$
BEGIN
    CALL uh_check_bucket(bucket);

    EXECUTE Format('SELECT pg_advisory_unlock_shared(hashtext(%L), hashtext(%L))',
        rel_name(bucket), key);
END
$$;
