#pragma once

#include <string>

bool has_sudo();

int run_with_optional_sudo(const std::string& command);
