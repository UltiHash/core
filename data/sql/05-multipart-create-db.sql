--
-- Create PostgreSQL multipart database
--
\c postgres;
DROP DATABASE IF EXISTS uh_multipart WITH (FORCE);
CREATE DATABASE uh_multipart;
GRANT ALL ON uh_multipart TO SESSION_USER;
