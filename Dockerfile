FROM ghcr.io/ultihash/build-base:20230118 as build

ARG CMAKE_OPTION

COPY . /core
WORKDIR /core

# Configure and compile
RUN mkdir build \
    && cmake -B build -D${CMAKE_OPTION}=ON -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j $(nproc) --config Release

# Execute tests
WORKDIR /core/build
RUN ctest -C Release --output-on-failure

FROM ubuntu:22.04 as deploy

ARG SRC_PATH
ARG TARGET

LABEL org.opencontainers.image.description="This container image contains a nightly snapshot of the ${SRC_PATH} role."

# Install curl to test if dependencies are already available (temporary workaround)
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
    && apt-get upgrade --yes \
    && apt-get install --yes --no-install-recommends curl ncat

COPY --from=build /core/build/${SRC_PATH}/${TARGET} /usr/local/bin
COPY --from=build /core/${SRC_PATH}/start.sh /usr/local/bin
RUN chmod +x /usr/local/bin/start.sh

RUN addgroup --system --gid 234 uh
RUN adduser --system --uid 234 --gid 234 --shell /bin/bash uh
USER uh
WORKDIR /home/uh

EXPOSE 21832
EXPOSE 8080

ENTRYPOINT ["start.sh"]
