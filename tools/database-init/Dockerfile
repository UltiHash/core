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

COPY --from=build /core/build/tools/access/uh-access /usr/local/bin/uh-cluster-access-client
COPY --from=build /core/build/bin/* /usr/local/bin

COPY --from=flyway /flyway /flyway

WORKDIR /flyway

COPY ./tools/database-init/apply_db_schema.sh .
COPY ./data/sql/ /flyway/migrations
    
ENTRYPOINT ["/bin/bash"]

CMD ["apply_db_schema.sh"]
