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

#include <entrypoint/http/request.h>
#include <entrypoint/http/stream.h>
#include <entrypoint/user/db.h>

#include <memory>

namespace uh::cluster::ep::http {

class request_factory {
public:
    request_factory(user::db& users);

    coro<std::unique_ptr<request>> create(stream& s, boost::asio::ip::tcp::endpoint peer);

private:
    user::db& m_users;
};

} // namespace uh::cluster::ep::http
