FROM ghcr.io/ultihash/build-base:20230418 as build

ARG CMAKE_OPTION

COPY . /core
WORKDIR /core

RUN wget -O /tmp/foundationdb-clients_7.1.31-1_amd64.deb https://github.com/apple/foundationdb/releases/download/7.1.31/foundationdb-clients_7.1.31-1_amd64.deb
RUN dpkg -i /tmp/foundationdb-clients_7.1.31-1_amd64.deb

# Configure and compile
RUN mkdir build \
    && cmake -B build -D${CMAKE_OPTION}=ON -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j $(nproc) --config Release

RUN wget -O /tmp/foundationdb-server_7.1.31-1_amd64.deb https://github.com/apple/foundationdb/releases/download/7.1.31/foundationdb-server_7.1.31-1_amd64.deb


# Execute tests
WORKDIR /core/build
RUN dpkg -i /tmp/foundationdb-server_7.1.31-1_amd64.deb && \
    ctest -C Release --output-on-failure

FROM ubuntu:22.04 as deploy

ARG SRC_PATH
ARG TARGET

LABEL org.opencontainers.image.description="This container image contains a nightly snapshot of the ${SRC_PATH} role."

# Install curl to test if dependencies are already available (temporary workaround)
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get upgrade --yes \
    && apt-get install --yes --no-install-recommends curl wget ncat

# Install FoundationDB
RUN if [ "$TARGET" = "uhServerAgency" ]; then \
        wget -O /tmp/foundationdb-clients_7.1.31-1_amd64.deb https://github.com/apple/foundationdb/releases/download/7.1.31/foundationdb-clients_7.1.31-1_amd64.deb && \
        dpkg -i /tmp/foundationdb-clients_7.1.31-1_amd64.deb; \
        wget -O /tmp/foundationdb-server_7.1.31-1_amd64.deb https://github.com/apple/foundationdb/releases/download/7.1.31/foundationdb-server_7.1.31-1_amd64.deb && \
        dpkg -i /tmp/foundationdb-server_7.1.31-1_amd64.deb; \
    fi

COPY --from=build /core/build/${SRC_PATH}/${TARGET} /usr/local/bin
COPY --from=build /core/${SRC_PATH}/start.sh /usr/local/bin
RUN chmod +x /usr/local/bin/start.sh

RUN addgroup --system --gid 234 uh
RUN adduser --system --uid 234 --gid 234 --shell /bin/bash uh

RUN mkdir /data
RUN chown -R uh:uh /data

# required for agency-node metrics persistence
RUN mkdir -p /var/lib/uh/agency-node
RUN chown -R uh:uh /var/lib/uh/agency-node

# required for database-node compression-queue persistence
RUN mkdir -p /var/lib/uh/data-node
RUN chown -R uh:uh /var/lib/uh/data-node

ADD foundationdb.conf /etc/foundationdb/foundationdb.conf
RUN mkdir -p /home/uh/foundationdb/log
RUN chown -R uh:uh /home/uh/foundationdb

USER uh
WORKDIR /home/uh

EXPOSE 21832
EXPOSE 8080

ENTRYPOINT ["start.sh"]
