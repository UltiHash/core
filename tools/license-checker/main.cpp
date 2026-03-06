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

#include <common/license/license.h>
#include <common/telemetry/log.h>

#include <CLI/CLI.hpp>

using namespace uh::cluster;

std::optional<license> read_config(int argc, char** argv) {
    CLI::App app("Upload test");
    argv = app.ensure_utf8(argv);

    license lic;

    app.add_option(
           "--license,-L",
           [&lic](CLI::results_t res) {
               try {
                   lic = license::create(res[0]);
               } catch (const std::exception& e) {
                   LOG_ERROR() << "parsing failed: " << e.what();
                   return false;
               }
               return true;
           },
           "UltiHash license json-string")
        ->envname("TEST_LICENSE")
        ->default_val(lic);

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    return lic;
}

uh::log::config
make_log_config(const boost::log::trivial::severity_level& log_level,
                const uh::cluster::role service_role) {
    uh::log::config lc;

    lc = {.sinks = {uh::log::sink_config{.type = uh::log::sink_type::cout,
                                         .level = log_level,
                                         .service_role = COORDINATOR_SERVICE}}};
    return lc;
}

int main(int argc, char** argv) {

    uh::log::config log_config =
        make_log_config(boost::log::trivial::debug, COORDINATOR_SERVICE);
    uh::log::init(log_config);

    try {
        auto lic = read_config(argc, argv);
        if (!lic) {
            std::cout << "license parse failed" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
