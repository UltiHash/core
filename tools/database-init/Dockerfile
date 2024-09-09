FROM flyway/flyway:10.17

RUN apt-get update -y && \
    apt-get install -y postgresql-client

COPY ./tools/database-init/apply_db_schema.sh .

COPY ./data/sql/ /flyway/migrations

ENTRYPOINT ["/bin/bash"]

CMD ["apply_db_schema.sh"]