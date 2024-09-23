#!/bin/bash

set -e

until pg_isready -U postgres -h $DB_HOST -p $DB_PORT; do echo 'Waiting for postgres...'; sleep 2; done

# Provision databases in advance
cat /flyway/migrations/provision_databases.sql | psql -U $DB_USER -h $DB_HOST -p $DB_PORT -d postgres

# Fill out the databases with their schemas
for database in `ls /flyway/migrations | grep -v .sql`
do
    flyway -locations="filesystem:/flyway/migrations/${database}" -url=jdbc:postgresql://$DB_HOST:$DB_PORT/$database -user="$DB_USER" -password="$PGPASSWORD" migrate
done

sleep infinity