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

#define BOOST_TEST_MODULE "global_data_view_fixture tests"

#include <boost/test/unit_test.hpp>

#include <util/gdv_fixture.h>

namespace uh::cluster {

BOOST_FIXTURE_TEST_CASE(test_fixture, global_data_view_fixture) {
    // Do nothing. I'd like to test it's constructor and destuctor only.
}

} // namespace uh::cluster
