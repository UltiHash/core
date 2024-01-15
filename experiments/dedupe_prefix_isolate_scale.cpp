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
    long max_step_back = static_cast <long> (min_fragment_size);

    uint8_t mmin_char;
    uint16_t mmax_char;

    [[nodiscard]] long get_match (const std::string_view& integration_data, size_t frag_size) const {

        if (integration_data.size() == frag_size or frag_size < min_fragment_size) {
            return 0;
        }

        long i = 0;
        bool matched = false;
        while (frag_size + i >= min_fragment_size and -i < max_step_back) {
            const auto step_back_index = frag_size + i + 1;
            const auto c = static_cast <uint8_t> (integration_data [step_back_index]);
            if (c < mmax_char and c >= mmin_char) {
                matched = true;
                break;
            }
            i --;
        }

        if (!matched) {
            return 1;
        }
        return i;
    }

public:

    dedupe_set (uint8_t min_char, uint16_t max_char): mmin_char (min_char), mmax_char (max_char) {

    }

    std::size_t dedupe (std::string_view& integration_data) {

        auto check_dedupe = [&](const std::string_view frag_data) {
            auto common_prefix = largest_common_prefix(integration_data, frag_data);

            auto step_back = get_match(integration_data, common_prefix);

            if (common_prefix >= min_fragment_size) {
                common_prefix = common_prefix + std::min (step_back + 1, 0l);

                integration_data = integration_data.substr(common_prefix);
                return std::pair {step_back, true};
            }
            return std::pair {step_back, false};
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
                if (res.second) {
                    continue;
                }
            }
            if (high != dset.cend()) {
                const auto &d = *high;
                auto res = check_dedupe(d);
                if (res.second) {
                    continue;
                }
            }

            auto frag_size = std::min(integration_data.size(), max_fragment_size);
            auto step_back = get_match(integration_data, frag_size);

            frag_size += std::min (step_back + 1, 0l);

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


    std::map <uint8_t, dedupe_set> sets;

    const auto range_size = static_cast <uint16_t> (std::ceil ((std::numeric_limits <uint8_t>::max() + 1 )/ static_cast <double> (set_count)));
    uint16_t offset = 0;
    for (size_t i = 0; i < set_count; ++i) {
        sets.emplace(offset, dedupe_set {static_cast <uint8_t> (offset), static_cast<uint16_t>(offset + range_size)});
        offset += range_size;
    }

    std::string_view integration_data (buf.data(), buf.size());

    std::map <uint8_t, size_t> space_savings;

    const auto part_size = static_cast <unsigned long> (std::ceil (static_cast <double> (integration_data.size()) / set_count));

    size_t effective_size = 0;
    size_t transfer_size = integration_data.size();
    while (!integration_data.empty()) {
        const auto adj_part_size = std::min (part_size, integration_data.size());
        auto data_part = integration_data.substr(0, adj_part_size);

        const auto& s = sets.upper_bound(static_cast <uint8_t> (data_part[0]));
        const auto set_effective_size = std::prev (s)->second.dedupe(data_part);
        const auto space_saved = adj_part_size - set_effective_size;
        if (const auto f = space_savings.find(std::prev (s)->first); f == space_savings.cend()) {
            space_savings.emplace_hint (f, std::prev (s)->first, space_saved);
        }
        else {
            f->second += space_saved;
        }
        effective_size += set_effective_size;
        integration_data = integration_data.substr(adj_part_size);

    }


    const auto constexpr GB = 1024.0 * 1024.0 * 1024.0;

    int i = 0;
    auto s = sets.cbegin();
    for (const auto& set_space_saving: space_savings) {
        std::cout << "set " << i++ << " fragment count " << s->second.get_size() << " space saving " << static_cast <double> (set_space_saving.second) / GB << " GB" << std::endl;
        s ++;
    }


    const auto space_saving = size - effective_size;

    std::cout << "original size: " << static_cast <double> (size) / GB << " GB" << std::endl;
    std::cout << "effective size: " << static_cast <double> (effective_size) / GB << " GB" << std::endl;
    std::cout << "space saved " << static_cast <double> (space_saving) / GB << " GB" << std::endl;
    std::cout << "space saving ratio " << static_cast <double> (space_saving) / static_cast <double> (size) << std::endl;
    std::cout << "transfer size: " << static_cast <double> (transfer_size) / GB << " GB" << std::endl;

}