#!/bin/bash
# Copyright 2026 UltiHash Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


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