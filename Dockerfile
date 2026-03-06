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

FROM ghcr.io/ultihash/build-base:latest AS build

ARG BuildType=Release

COPY . /core
WORKDIR /core

# Configure and compile
RUN mkdir build \
    && cmake -B build -DBUILD_TESTS=OFF -DBUILD_TOOLS=OFF -DCMAKE_BUILD_TYPE=${BuildType} \
    && cmake --build build -j $(nproc) --config ${BuildType}

FROM ubuntu:24.04 AS deploy

ARG DebugTools=False

LABEL org.opencontainers.image.description="UltiHash is an S3-compatible, purpose-built object storage for AI and advanced analytics, reducing storage by up to 60% with byte-level deduplication - all while maintaining high throughput for large-scale ops."

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
   && apt-get upgrade --yes \
   && apt-get install --yes --no-install-recommends libgrpc29t64 libgrpc++1.51t64 libpq5 liblmdb0 adduser ca-certificates \
   && if [ "$DebugTools" = "True" ]; then \
        apt-get install --yes --no-install-recommends net-tools gdb gdbserver; \
    fi

COPY --from=build /core/build/bin/* /usr/local/bin

RUN addgroup --system --gid 234 uh
RUN adduser --system --uid 234 --gid 234 --shell /bin/bash uh

# create working directory
RUN install -d -m 0755 -o uh -g uh /var/lib/uh

USER uh
WORKDIR /home/uh
