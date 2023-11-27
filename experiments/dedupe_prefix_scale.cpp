//
// Created by massi on 11/27/23.
//

#include <set>
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <string_view>
#include <optional>


static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
    if (str1.size() <= str2.size()) {
        return std::distance(str1.cbegin(), std::mismatch(str1.cbegin(), str1.cend(), str2.cbegin()).first);
    }
    else {
        return std::distance(str2.cbegin(), std::mismatch(str2.cbegin(), str2.cend(), str1.cbegin()).first);
    }
}

int main (int argc, char* args[]) {

    size_t min_fragment_size = 32ul;
    size_t max_fragment_size = 8ul * 1024ul;

    std::filesystem::path p (args[0]);
    std::fstream file (p);
    std::vector <char> buf (std::filesystem::file_size(p));
    file.read(buf.data(), static_cast <long> (buf.size()));

    std::set <std::string_view> set1;
    std::set <std::string_view> set2;

    std::string_view integration_data (buf.data(), buf.size());

    auto check_dedupe = [&] (const std::string_view frag_data) {
        auto common_prefix = largest_common_prefix(integration_data, frag_data);
        if (common_prefix >= min_fragment_size) {
            integration_data = integration_data.substr(common_prefix);
            return true;
        }
        return false;
    };

    size_t effective_size = 0;

    while (!integration_data.empty()) {
        const auto high = set1.find (integration_data);
        std::optional <decltype(high)> low;
        if (high != set1.cbegin()) {
            low.emplace(std::prev(high));
        }

        if (low.has_value()) {
            const auto& d = *low.value();
            if (check_dedupe (d)) {
                continue;
            }
        }
        if (high != set1.cend()) {
            const auto& d = *high;
            if (check_dedupe (d)) {
                continue;
            }
        }

        const auto frag_size = std::min (integration_data.size (), max_fragment_size);
        set1.emplace_hint(high, integration_data.substr(0, frag_size));
        effective_size += frag_size;
        integration_data = integration_data.substr(frag_size);
    }

    std::cout << effective_size << std::endl;

}