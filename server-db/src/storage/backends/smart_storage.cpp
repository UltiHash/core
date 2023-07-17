//
// Created by masi on 5/30/23.
//
#include "smart_storage.h"
#include <licensing/global_licensing.h>

namespace uh::dbn::storage::smart {

smart_config make_smart_config(const std::filesystem::path &root, size_t size, size_t max_file_size) {

    constexpr unsigned long GB = 1024ul * 1024ul * 1024ul;

    const std::filesystem::path data_store_directory = root / "data_store";
    const std::filesystem::path set_directory = root / "set";
    const std::filesystem::path hash_table_directory = root / "hash_table";

    // data store
    fixed_managed_storage_config ds_conf;
    ds_conf.data_store_file_size = 16ul * GB;
    ds_conf.log_file = data_store_directory / "log_file";
    size_t offset = 0;
    while (offset < 32ul * GB) {
        const std::filesystem::path data_store_file = "data_" + std::to_string(offset);
        ds_conf.data_store_files.emplace_front (data_store_directory / data_store_file);
        offset += ds_conf.data_store_file_size;
    }

    dedupe_config dd_conf {};
    dd_conf.min_fragment_size = 2 * 1024;
    dd_conf.max_fragment_size = 16 * 1024;


    // fragment set
    growing_plain_storage_config fragment_set_key_store;
    fragment_set_key_store.init_size = 2ul * GB;
    fragment_set_key_store.file = set_directory / "fragment_set";

    set_config fragment_set_conf;
    fragment_set_conf.max_empty_hole_size = 1ul * GB;
    fragment_set_conf.set_minimum_free_space = GB;
    fragment_set_conf.key_store_config = std::move (fragment_set_key_store);

    // key store sorted map
    growing_plain_storage_config key_store_index_set;
    key_store_index_set.init_size = 2ul * GB;
    key_store_index_set.file = set_directory / "key_store_index";

    growing_managed_storage_config key_store_data_store;
    key_store_data_store.max_file_size = 1ul * GB;
    key_store_data_store.min_file_size = GB;
    key_store_data_store.log_file = set_directory / "log";
    key_store_data_store.directory = set_directory / "key_store_data";

    set_config key_store_set_conf;
    key_store_set_conf.key_store_config = std::move (key_store_index_set);
    key_store_set_conf.max_empty_hole_size = 1ul * GB;
    key_store_set_conf.set_minimum_free_space = GB;

    sorted_map_config sorted_map_conf;
    sorted_map_conf.index_store = std::move (key_store_set_conf);
    sorted_map_conf.data_store = std::move (key_store_data_store);

    // key store hashmap
    growing_plain_storage_config key_store_hashmap_key_store;
    key_store_hashmap_key_store.init_size = 4ul * GB;
    key_store_hashmap_key_store.file = hash_table_directory / "key_file";

    growing_managed_storage_config key_store_hashmap_value_store;
    key_store_hashmap_value_store.directory = hash_table_directory / "values";
    key_store_hashmap_value_store.log_file = key_store_hashmap_value_store.directory / "log";
    key_store_hashmap_value_store.min_file_size = 4ul * GB;
    key_store_hashmap_value_store.max_file_size = 8ul * GB;

    hashmap_config hashmap_conf;
    hashmap_conf.key_size = 64;
    hashmap_conf.map_load_factor = 0.9;
    hashmap_conf.map_maximum_extension_factor = 32;
    hashmap_conf.key_store_config = std::move (key_store_hashmap_key_store);
    hashmap_conf.value_store_config = std::move (key_store_hashmap_value_store);

    return {hashmap_conf, fragment_set_conf, sorted_map_conf, ds_conf, dd_conf};
}
} // end namespace uh::dbn::storage::smart



namespace uh::dbn::storage {

smart_storage::smart_storage(const smart::smart_config &smart_conf, uh::dbn::metrics::storage_metrics& storage_metrics) :
        m_smart_conf (smart_conf),
        m_smart_core (smart_conf),
        m_size (smart_conf.data_store_conf.data_store_file_size * smart_conf.data_store_conf.data_store_files.size()),
        m_sha_ctx (EVP_MD_CTX_create ()),
        m_storage_metrics (storage_metrics) {
    EVP_DigestInit_ex (m_sha_ctx, EVP_sha512 (), nullptr);
}

std::unique_ptr<io::data_generator> smart_storage::read_block (const std::span<char> &hash) {
    std::shared_lock <std::shared_mutex> lock (m_mutex);
    auto fragments_size_pair = m_smart_core.retrieve(hash);
    return std::make_unique <io::span_generator> (fragments_size_pair.first, std::move (fragments_size_pair.second));
}

std::pair <std::size_t, std::vector <char>> smart_storage::write_block (const std::span <const char>& data) {
    std::vector <char> sha (m_smart_conf.hashmap_key_store_conf.key_size);
    unsigned int size;
    std::lock_guard <std::shared_mutex> lock (m_mutex);
    EVP_DigestUpdate (m_sha_ctx, data.data(), data.size());
    EVP_DigestFinal_ex (m_sha_ctx, reinterpret_cast <unsigned char*> (sha.data()), &size);
    std::pair <std::uint8_t, std::size_t> res;
    try {
        m_used += data.size();
        uh::dbn::licensing::global_license_pointer_dbn->license_package()
            .require(uh::licensing::feature::STORAGE, m_used);
        res = m_smart_core.integrate({sha.data(), size}, std::string_view(data.data(), data.size()), util::insertion_type::INSERT_IGNORE);
        update_space_consumption();
    } catch (std::exception& e) {
        m_used -= data.size();
        throw e;
    }
    EVP_DigestInit_ex (m_sha_ctx, EVP_sha512 (), nullptr);
    return {res.second, std::move (sha)};
}


std::pair <std::uint8_t, std::size_t> smart_storage::write_key_value(const std::span<char> &key, const std::span<char> &data, util::insertion_type insert_type) {
    std::pair <std::uint8_t, std::size_t> res;

    try {
        std::lock_guard <std::shared_mutex> lock (m_mutex);
        m_used += data.size();
        res = m_smart_core.integrate(key, std::string_view(data.data(), data.size()), insert_type);
        INFO << "Maximum unmatched common prefix size less than minimum fragment size: " << m_smart_core.max_common << std::endl;
        m_smart_core.max_common = 0;
        update_space_consumption();
    } catch (std::exception& e) {
        m_used -= data.size();
        throw e;
    }
    return res;
}

size_t smart_storage::free_space() {
    return m_size - m_used;
}

size_t smart_storage::used_space() {
    return m_used;
}

size_t smart_storage::allocated_space() {
    return m_size;
}

std::string smart_storage::backend_type() {
    return std::string (m_type);
}

void smart_storage::start() {
    INFO << "--- Storage backend initialized --- " << std::filesystem::absolute(m_smart_conf.data_store_conf.data_store_files.front().parent_path());
    INFO << "        backend type   : " << backend_type();
    INFO << "        root directory : " << std::filesystem::absolute(m_smart_conf.data_store_conf.data_store_files.front().parent_path());
    INFO << "        space allocated: " << allocated_space();
    INFO << "        space available: " << free_space();
    INFO << "        space consumed : " << used_space();
}

void smart_storage::stop()
{
    INFO << "smart storage stopped";
}

void smart_storage::update_space_consumption() {
    m_storage_metrics.alloc_space().Set(m_size);
    m_storage_metrics.free_space().Set(m_size - m_used);
    m_storage_metrics.used_space().Set(m_used);
}

std::list<key_value_generator>
smart_storage::fetch_query(const std::span<char> &start_key, const std::span<char> &end_key,
                           const std::span<std::string_view> &labels) {

    std::shared_lock <std::shared_mutex> lock (m_mutex);

    auto key_fragmented_values = m_smart_core.retrieve_range(start_key, end_key, labels);

    std::list <key_value_generator> result;

    for (auto& key_fragmented_val: key_fragmented_values) {
        auto key_span_generator = std::make_unique<io::span_generator>(key_fragmented_val.first.size(), std::forward_list<std::span<char>> {key_fragmented_val.first});
        auto value_span_generator = std::make_unique<io::span_generator>(key_fragmented_val.second.first, std::move (key_fragmented_val.second.second));

        result.push_front({std::move (key_span_generator), std::move (value_span_generator)});
    }
    return result;
}

std::unique_ptr<io::data_generator> smart_storage::read_value (const std::span <char>& key, const std::span <std::string_view>& labels) {
    std::shared_lock <std::shared_mutex> lock (m_mutex);
    auto fragments_size_pair = m_smart_core.retrieve(key);
    // fragments_size_pair.second.emplace_front(key);
    return std::make_unique <io::span_generator> (fragments_size_pair.first, std::move (fragments_size_pair.second));
}


} // end namespace uh::dbn::storage
