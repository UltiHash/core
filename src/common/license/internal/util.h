#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace uh::cluster {

bool verify_license(std::string_view data, const std::vector<char>& signature);

void throw_from_openssl_error(const std::string& prefix);

} // namespace uh::cluster
