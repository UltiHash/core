FROM ghcr.io/ultihash/build-base:20230118 as build

COPY . /uhClient
WORKDIR /uhClient

# Configure and compile lib-common
RUN mkdir build \
    && cmake -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j $(nproc) --config Release

RUN cd /uhClient/build && ctest -C Release --output-on-failure

# Execute tests
RUN ctest -C Release --output-on-failure

FROM ubuntu:22.04 as deploy
LABEL org.opencontainers.image.description "This container image contains the UltiHash CLI client. \
    The entry-point of the image is test script which integrates and then recompiles the client binary."

COPY --from=build /uhClient/build/uhClient /usr/local/bin
COPY --from=build /uhClient/start.sh /usr/local/bin
RUN chmod +x /usr/local/bin/start.sh

RUN useradd -m ultihash
USER ultihash
WORKDIR /home/ultihash

ENTRYPOINT ["start.sh"]