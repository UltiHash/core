#ifndef CORE_BIG_INT_H
#define CORE_BIG_INT_H

#include <cstdint>
#include <functional>
#include <numeric>
#include <string>

namespace uh::cluster {

class big_int {
    uint64_t num[2]; // 0 = high, 1 = low
  public:
    constexpr big_int() noexcept : num{0, 0} {}

    constexpr big_int(unsigned long number) noexcept : num{0, number} {}
    constexpr big_int(unsigned long nh, unsigned long nl) noexcept
        : num{nh, nl} {}

    explicit big_int(const std::string& num_str) : big_int() {
        const auto index = num_str.find('_') + 1;
        const auto num0_str = num_str.substr(0, index - 1);
        const auto num1_str = num_str.substr(index, num_str.size());
        num[0] = std::stoul(num0_str);
        num[1] = std::stoul(num1_str);
    }

    auto operator<=>(const big_int&) const = default;

    constexpr inline big_int& operator+=(const big_int& other) noexcept {
        num[0] += other.num[0];
        num[1] += other.num[1];

        if (other.num[1] > num[1]) [[unlikely]] {
            num[0]++;
        }

        return *this;
    }

    constexpr inline big_int operator+(const big_int& other) const noexcept {
        return big_int(*this) += other;
    }

    constexpr inline big_int operator-(const big_int& other) const noexcept {
        big_int res{num[0] - other.num[0], num[1] - other.num[1]};

        if (other.num[1] > num[1]) [[unlikely]] {
            res.num[0]--;
        }

        return res;
    }

    constexpr inline big_int
    operator*(const unsigned long other) const noexcept {
        const unsigned long sub_num11 = num[1] & UNSIGNED_MAX_4;
        const unsigned long sub_num10 = num[1] >> 32;
        const unsigned long sub_num01 = num[0] & UNSIGNED_MAX_4;
        const unsigned long sub_num00 = num[0] >> 32;
        const unsigned long sub_other1 = other & UNSIGNED_MAX_4;
        const unsigned long sub_other0 = other >> 32;

        const auto mul111 = sub_other1 * sub_num11; // << 0
        const auto mul110 = sub_other1 * sub_num10; // << 32
        const auto mul101 = sub_other1 * sub_num01; // << 64
        const auto mul100 = sub_other1 * sub_num00; // << 96

        const auto mul011 = sub_other0 * sub_num11; // << 32
        const auto mul010 = sub_other0 * sub_num10; // << 64
        const auto mul001 = sub_other0 * sub_num01; // << 96
        // const auto mul000 = sub_other0 * sub_num00; // << 128 -- overflow

        const auto res0 = mul111;
        const auto res32 = mul110 + mul011;
        const auto res64 = mul101 + mul010;
        const auto res96 = mul100 + mul001;

        auto num0 = res64 + (res96 << 32) + (res32 >> 32);
        const auto lshifted_res32 = (res32 << 32);
        const auto num1 = res0 + lshifted_res32;

        if (const auto max_no_overflow = UNSIGNED_MAX_8 - res0;
            max_no_overflow < lshifted_res32) [[unlikely]] {
            num0++;
        }

        if (const auto max_no_overflow = UNSIGNED_MAX_8 - mul110;
            max_no_overflow < mul011) [[unlikely]] {
            num0 += (1ul << 32);
        }

        return {num0, num1};
    }

    [[nodiscard]] inline std::string to_string() const {
        return std::to_string(num[0]) + "_" + std::to_string(num[1]);
    }

    [[nodiscard]] constexpr inline uint64_t get_high() const noexcept {
        return num[0];
    }

    [[nodiscard]] constexpr inline uint64_t get_low() const noexcept {
        return num[1];
    }

    [[nodiscard]] constexpr inline const auto& get_data() const noexcept {
        return num;
    }

    constexpr inline auto& ref_data() noexcept { return num; }

    constexpr static auto UNSIGNED_MAX_8 =
        std::numeric_limits<unsigned long>::max();
    constexpr static auto UNSIGNED_MAX_4 = std::numeric_limits<uint32_t>::max();
};

constexpr big_int operator+(uint64_t a, const big_int& b) {
    return big_int(a) + b;
}

constexpr big_int operator-(uint64_t a, const big_int& b) {
    return big_int(a) - b;
}

typedef big_int uint128_t;

} // end namespace uh::cluster

template <> struct std::hash<uh::cluster::big_int> {
    std::size_t operator()(const uh::cluster::big_int& num) const {
        const auto hash_func = std::hash<uint64_t>();
        return hash_func(num.get_data()[0] + num.get_data()[1]);
    }
};

#endif // CORE_BIG_INT_H
