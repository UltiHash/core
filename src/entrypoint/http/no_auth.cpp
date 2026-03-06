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

#include "no_auth.h"

#include "chunked_body.h"
#include "raw_body.h"

namespace uh::cluster::ep::http {

coro<std::unique_ptr<request>> no_auth::create(stream& s, raw_request req) {

    if (req.optional("Transfer-Encoding").value_or("") == "chunked") {
        auto body = std::make_unique<chunked_body>(s);
        co_return std::make_unique<request>(
            std::move(req), std::move(body),
            user::user{.name = user::user::ANONYMOUS});
    } else {
        auto body = std::make_unique<raw_body>(s, req);
        co_return std::make_unique<request>(
            std::move(req), std::move(body),
            user::user{.name = user::user::ANONYMOUS});
    }
}

} // namespace uh::cluster::ep::http
