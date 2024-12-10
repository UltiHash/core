#pragma once

#include <random>
#include <string>

std::string generate_random_string(size_t length) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 255);

    std::string random_string(length, '\0');
    for (size_t i = 0; i < length; ++i) {
        random_string[i] = static_cast<char>(distribution(generator));
    }
    return random_string;
}
