#include "system.h"

#include <cstdlib>

bool has_sudo() { return std::system("which sudo > /dev/null 2>&1") == 0; }

int run_with_optional_sudo(const std::string& command) {
    std::string full_command = has_sudo() ? "sudo " + command : command;
    return std::system(full_command.c_str());
}
