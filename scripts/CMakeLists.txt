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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/reset-db.sh.in ${CMAKE_CURRENT_BINARY_DIR}/reset-db.sh.tmp)
file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/reset-db.sh
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/reset-db.sh.tmp
    USE_SOURCE_PERMISSIONS)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/start.sh.in ${CMAKE_CURRENT_BINARY_DIR}/start.sh.tmp @ONLY)
file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/start.sh
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/start.sh.tmp
    USE_SOURCE_PERMISSIONS)
