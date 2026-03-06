/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>

namespace uh::cluster::db {

struct config {

    // host and port of database
    std::string host_port = DEFAULT_HOST_PORT;

    // db user name
    std::string username = DEFAULT_USER;

    // db password
    std::string password = DEFAULT_PASS;

    struct database {
        std::string dbname;
        unsigned count;
    };

    database directory = {"uh_directory", 2u};
    database multipart = {"uh_multipart", 2u};
    database users = {"uh_user", 2u};

    static constexpr const char* DEFAULT_HOST_PORT = "localhost:5432";
    static constexpr const char* DEFAULT_USER = "";
    static constexpr const char* DEFAULT_PASS = "";
};

} // namespace uh::cluster::db
