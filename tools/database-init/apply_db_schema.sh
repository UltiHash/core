#!/bin/bash

set -e

# Provision databases in advance
cat /flyway/migrations/provision_databases.sql | psql -U $DB_USER -h $DB_HOST -p $DB_PORT -d postgres

# Fill out the databases with their schemas
for database in `ls /flyway/migrations | grep -v .sql`
do
    flyway -locations="filesystem:/flyway/migrations/${database}" -url=jdbc:postgresql://$DB_HOST:$DB_PORT/$database -user="$DB_USER" -password="$PGPASSWORD" migrate
done

# Set up the super user
uh-cluster-access-client user-add --db-host $DB_HOST:$DB_PORT --db-user $DB_USER --db-pass $PGPASSWORD --superuser --if-not-exists $SUPER_USER_USERNAME
uh-cluster-access-client key-add --db-host $DB_HOST:$DB_PORT --db-user $DB_USER --db-pass $PGPASSWORD --if-not-exists $SUPER_USER_USERNAME $SUPER_USER_ACCESS_KEY_ID $SUPER_USER_SECRET_KEY

