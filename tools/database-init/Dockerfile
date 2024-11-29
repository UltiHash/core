FROM ghcr.io/ultihash/build-base:latest AS build

COPY . /core
WORKDIR /core

RUN mkdir build \
    && CC=/usr/bin/gcc CXX=/usr/bin/g++ cmake -G Ninja -B build -DBUILD_TESTS=OFF -DBUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j $(nproc) --config Release

FROM flyway/flyway:10.21 AS flyway

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \                               
    apt-get install -y openjdk-17-jdk postgresql-client libgrpc-dev libgrpc++-dev liblmdb-dev liblmdb++-dev  

COPY --from=build /core/build/tools/uh-cluster-access-client /usr/local/bin
COPY --from=build /core/build/libuh-cluster_shared.so /usr/local/bin

COPY --from=flyway /flyway /flyway

WORKDIR /flyway

COPY ./tools/database-init/apply_db_schema.sh .
COPY ./data/sql/ /flyway/migrations
    
ENTRYPOINT ["/bin/bash"]

CMD ["apply_db_schema.sh"]
