FROM ghcr.io/ultihash/build-base:20240711 AS build

COPY . /core
WORKDIR /core

RUN mkdir build \
    && cmake -B build -DBUILD_TESTS=OFF -DBUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j $(nproc) --config Release

FROM flyway/flyway:10.17

RUN apt-get update -y && \
    apt-get install -y postgresql-client libgrpc-dev libgrpc++-dev liblmdb-dev liblmdb++-dev


COPY --from=build /core/build/tools/uh-cluster-access-client /usr/local/bin

COPY ./tools/database-init/apply_db_schema.sh .

COPY ./data/sql/ /flyway/migrations

ENTRYPOINT ["/bin/bash"]

CMD ["apply_db_schema.sh"]