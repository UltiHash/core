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
#include <list>
#include <cmath>
#include <map>


static size_t largest_common_prefix(const std::string_view &str1, const std::string_view &str2) noexcept {
    if (str1.size() <= str2.size()) {
        return std::distance(str1.cbegin(), std::mismatch(str1.cbegin(), str1.cend(), str2.cbegin()).first);
    }
    else {
        return std::distance(str2.cbegin(), std::mismatch(str2.cbegin(), str2.cend(), str1.cbegin()).first);
    }
}

class dedupe_set {
    std::set <std::string_view> dset;

    size_t min_fragment_size = 32ul;
    size_t max_fragment_size = 8ul*1024ul;

public:

    std::size_t dedupe (std::string_view& integration_data) {

        auto check_dedupe = [&](const std::string_view frag_data) {
            auto common_prefix = largest_common_prefix(integration_data, frag_data);

            if (common_prefix >= min_fragment_size) {
                integration_data = integration_data.substr(common_prefix);
                return true;
            }
            return false;
        };

        size_t effective_size = 0;

        while (!integration_data.empty()) {
            const auto high = dset.lower_bound(integration_data);
            std::optional<decltype(high)> low;
            if (high != dset.cbegin()) {
                low.emplace(std::prev(high));
            }

            if (low.has_value()) {
                const auto &d = *low.value();
                auto res = check_dedupe(d);
                if (res) {
                    continue;
                }
            }
            if (high != dset.cend()) {
                const auto &d = *high;
                auto res = check_dedupe(d);
                if (res) {
                    continue;
                }
            }

            auto frag_size = std::min(integration_data.size(), max_fragment_size);

            dset.emplace_hint(high, integration_data.substr(0, frag_size));
            effective_size += frag_size;
            integration_data = integration_data.substr(frag_size);

        }

        return effective_size;
    }

    [[nodiscard]] size_t get_size () const {
        return dset.size();
    }
};


int main (int argc, char* args[]) {


    const auto set_count = std::strtoul(args[1], nullptr, 10);

    std::filesystem::path p ((args[2]));
    std::fstream file (p, std::ios::in | std::ios::binary);
    if (!file or !std::filesystem::exists(p)) {
        perror ("file not open!");
        throw std::runtime_error ("Could not open the file in " + p.string());
    }
    auto size = std::filesystem::file_size(p);
    std::vector <char> buf (std::filesystem::file_size(p));
    file.read(buf.data(), static_cast <long> (buf.size()));


    std::vector <dedupe_set> sets (set_count);

    std::string_view integration_data (buf.data(), buf.size());

    size_t effective_size = 0;
    size_t transfer_size = integration_data.size();

    const auto part_size = static_cast <unsigned long> (std::ceil (static_cast <double> (integration_data.size()) / set_count));
    std::map <int, size_t> space_savings;

    size_t offset = 0;
    int set_id = 0;
    while (offset < integration_data.size()) {

        const auto adj_part_size = std::min (part_size, integration_data.size());
        auto data_part = integration_data.substr(0, adj_part_size);
        const auto set_effective_size = sets[set_id].dedupe(data_part);
        effective_size += set_effective_size;
        integration_data = integration_data.substr(adj_part_size);

        space_savings[set_id] += adj_part_size - set_effective_size;

        set_id ++;
        set_id = set_id % sets.size();
    }

    const auto constexpr GB = 1024.0 * 1024.0 * 1024.0;
    const auto space_saving = size - effective_size;

    int i = 0;
    for (const auto& s: sets) {
        std::cout << "set " << i << " fragment count " << s.get_size() << " space saving " << static_cast <double> (space_savings[i]) / GB << " GB" << std::endl;
        i++;
    }

    std::cout << "original size: " << static_cast <double> (size) / GB << " GB" << std::endl;
    std::cout << "effective size: " << static_cast <double> (effective_size) / GB << " GB" << std::endl;
    std::cout << "space saved " << static_cast <double> (space_saving) / GB << " GB" << std::endl;
    std::cout << "space saving ratio " << static_cast <double> (space_saving) / static_cast <double> (size) << std::endl;
    std::cout << "transfer size: " << static_cast <double> (transfer_size) / GB << " GB" << std::endl;

}