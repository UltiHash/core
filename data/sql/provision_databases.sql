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
-- Create PostgreSQL directory database
--
CREATE DATABASE uh_directory;
GRANT ALL ON DATABASE uh_directory TO SESSION_USER;

--
-- Create PostgreSQL multipart database
--
CREATE DATABASE uh_multipart;
GRANT ALL ON DATABASE uh_multipart TO SESSION_USER;

--
-- Create PostgreSQL user database
--
CREATE DATABASE uh_user;
GRANT ALL ON DATABASE uh_user TO SESSION_USER;
