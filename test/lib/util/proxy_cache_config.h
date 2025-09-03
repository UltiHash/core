#pragma once

#include <string>

namespace uh::cluster::proxy::cache {

struct s3_object_key {
    std::string bucket_name;
    std::string object_name;
    std::string version;

    bool operator==(const s3_object_key& other) const {
        return bucket_name == other.bucket_name &&
               object_name == other.object_name && version == other.version;
    }
};

} // namespace uh::cluster::proxy::cache

template <> struct std::hash<uh::cluster::proxy::cache::s3_object_key> {
    size_t
    operator()(const uh::cluster::proxy::cache::s3_object_key& key) const {
        std::size_t seed = 0;

        auto hash_combine = [](std::size_t& seed, const auto& v) {
            seed ^= std::hash<std::remove_cvref_t<decltype(v)>>{}(v) +
                    0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        };

        hash_combine(seed, key.bucket_name);
        hash_combine(seed, key.object_name);
        hash_combine(seed, key.version);

        return seed;
    }
};

namespace uh::cluster::proxy::cache {

template <typename T = int> struct vector_entry {
    std::vector<T> value;

    vector_entry(std::vector<T>&& v)
        : value(std::move(v)) {}

    vector_entry(std::initializer_list<T> il)
        : vector_entry(std::vector<T>(il)) {}

    std::size_t data_size() const { return value.size() * sizeof(T); }

    T& operator[](std::size_t i) { return value[i]; }
    const T& operator[](std::size_t i) const { return value[i]; }

    static std::shared_ptr<vector_entry<T>> create(std::vector<T>&& v) {
        return std::make_shared<vector_entry<T>>(std::move(v));
    }
};

using char_vector = vector_entry<char>;

} // namespace uh::cluster::proxy::cache
