FROM ghcr.io/ultihash/build-base:20230118 as build

COPY . /uhServerAgency
WORKDIR /uhServerAgency

# Configure and compile lib-common
RUN mkdir build \
    && cmake -B build -DCMAKE_BUILD_TYPE=Debug \
    && cmake --build build -j $(nproc) --config Debug

RUN cd /uhServerAgency/build && ctest -C Debug --output-on-failure

FROM ubuntu:22.04 as deploy
LABEL org.opencontainers.image.description "This container image contains a nightly snapshot of the agency node server role. \
The ```start.sh``` script is used as the default entry point with no additional parameters specified. \
With this, the AN server role listens on port 21832, and the prometheus-cpp exporter listens on port 8080."

# Install curl to test if dependencies are already available (temporary workaround)
RUN DEBIAN_FRONTEND=noninteractive apt-get update \
    && apt-get upgrade --yes \
    && apt-get install --yes --no-install-recommends curl

COPY --from=build /uhServerAgency/build/uhServerAgency /usr/local/bin
COPY --from=build /uhServerAgency/start.sh /usr/local/bin
RUN chmod +x /usr/local/bin/start.sh

RUN useradd -m ultihash
USER ultihash
WORKDIR /home/ultihash

EXPOSE 21832
EXPOSE 8080

ENTRYPOINT ["start.sh"]