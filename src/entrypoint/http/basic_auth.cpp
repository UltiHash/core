// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "basic_auth.h"

#include "chunked_body.h"
#include "raw_body.h"
#include <common/utils/strings.h>

namespace uh::cluster::ep::http {

coro<std::unique_ptr<request>>
basic_auth::create(stream& s, user::db& users, raw_request req) {

    auto header = req.require("authorization");
    std::size_t pos = header.find(' ');
    if (pos == std::string::npos) {
        throw std::runtime_error("no algorithm separator");
    }

    auto decoded = base64_decode({header.begin() + pos + 1, header.end()});
    auto creds = split({decoded.begin(), decoded.end()}, ':');

    if (creds.size() != 2) {
        throw std::runtime_error("credentials format error");
    }

    auto user = co_await users.find_and_check(std::string(creds[0]),
                                              std::string(creds[1]));

    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        auto body = std::make_unique<chunked_body>(s);
        co_return std::make_unique<request>(std::move(req), std::move(body),
                                            std::move(user));
    } else {
        auto body = std::make_unique<raw_body>(s, req);
        co_return std::make_unique<request>(std::move(req), std::move(body),
                                            std::move(user));
    }
}

} // namespace uh::cluster::ep::http
