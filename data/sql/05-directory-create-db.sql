--
-- Create PostgreSQL directory database
--
\c postgres;
DROP DATABASE IF EXISTS uh_directory WITH (FORCE);
CREATE DATABASE uh_directory;
GRANT ALL ON DATABASE uh_directory TO SESSION_USER;
\c uh_directory;
