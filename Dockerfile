FROM ghcr.io/ultihash/build-base:latest as build

COPY . /core
WORKDIR /core

# Configure and compile
RUN mkdir build \
    && cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j $(nproc) --config Release

# Execute tests
WORKDIR /core/build
RUN etcd & \
    sleep 5 && \
    ctest -C Release --output-on-failure

FROM ubuntu:22.04 as deploy

LABEL org.opencontainers.image.description="This container image contains a nightly snapshot of the UltiHash object storage cluster."

# Install curl to test if dependencies are already available (temporary workaround)
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get upgrade --yes \
    && apt-get install --yes --no-install-recommends libpugixml1v5 libgrpc10 libgrpc++1 lsof

COPY --from=build /core/build/uh-cluster /usr/local/bin
COPY --from=build /core/build/lib/third-party/etcd-cpp-apiv3/src/libetcd-cpp-api.so /usr/local/lib
COPY --from=build /usr/local/lib/libcpprest.so.2.10 /usr/local/lib

RUN addgroup --system --gid 234 uh
RUN adduser --system --uid 234 --gid 234 --shell /bin/bash uh

# create working directory
RUN install -d -m 0755 -o uh -g uh /var/lib/uh

USER uh
WORKDIR /home/uh