FROM ghcr.io/ultihash/build-base:20230118 as build

COPY . /uhServerDB
WORKDIR /uhServerDB

# Configure and compile server-db
RUN mkdir build \
    && cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build build -j $(nproc) --config Debug

# Execute tests
RUN cd /uhServerDB/build && ctest -C Debug --output-on-failure


FROM ubuntu:22.04 as deploy
LABEL org.opencontainers.image.description "This container image contains the database node server role. \
The ```uhServerDB``` binary is used as the default entry point and is supplied with the parameters \
```--db-root . --create-new-root --threads 15 --port 12345``. \
With these settings, the DB server role listens on port 12345, and the prometheus-cpp exporter listens on port 8080."

COPY --from=build /uhServerDB/build/uhServerDB /usr/local/bin

RUN useradd -m ultihash
USER ultihash
WORKDIR /home/ultihash

EXPOSE 12345
EXPOSE 8080

ENTRYPOINT [ "uhServerDB", "--db-root", ".", "--create-new-root", "--threads", "15", "--port", "12345"]