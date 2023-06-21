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

    data_store_config ds_conf;
    ds_conf.data_store_file_size = 4ul * GB;
    ds_conf.log_file = data_store_directory / "log_file";
    size_t offset = 0;
    while (offset < 20ul * GB) {
        const std::filesystem::path data_store_file = "data_" + std::to_string(offset);
        ds_conf.data_store_files.emplace_front (data_store_directory / data_store_file);
        offset += ds_conf.data_store_file_size;
    }

    dedupe_config dd_conf {};
    dd_conf.min_fragment_size = 2 * 1024;

    set_config set_conf;
    set_conf.set_init_file_size = 2ul * GB;
    set_conf.max_empty_hole_size = 1ul * GB;
    set_conf.set_minimum_free_space = GB;

    set_conf.fragment_set_path = set_directory / "fragment_set";

    smart::map_config map_conf;
    map_conf.key_size = 64;
    map_conf.map_key_file_init_size = 4ul * GB;
    map_conf.map_values_minimum_file_size = 4ul * GB;
    map_conf.map_values_maximum_file_size = 8ul * GB;
    map_conf.map_load_factor = 0.9;
    map_conf.map_maximum_extension_factor = 32;
    map_conf.hashtable_key_path = hash_table_directory / "key_file";
    map_conf.hashtable_value_directory = hash_table_directory / "values";
    map_conf.value_store_log_file = map_conf.hashtable_value_directory / "log";

    return {map_conf, set_conf, ds_conf, dd_conf};
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

std::pair <std::size_t, std::vector <char>> smart_storage::write_block (const std::span <char>& data) {
    std::vector <char> sha (m_smart_conf.map_conf.key_size);
    unsigned int size;
    std::lock_guard <std::shared_mutex> lock (m_mutex);
    EVP_DigestUpdate (m_sha_ctx, data.data(), data.size());
    EVP_DigestFinal_ex (m_sha_ctx, reinterpret_cast <unsigned char*> (sha.data()), &size);
    std::size_t effective_size;
    try {
        m_used += data.size();
        uh::dbn::licensing::global_license_pointer_dbn->license_package()
            .allocate(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY, data.size());
        effective_size = m_smart_core.integrate({sha.data(), size}, std::string_view(data.data(), data.size()));
        update_space_consumption();
    } catch (std::exception& e) {
        m_used -= data.size();
        throw e;
    }
    EVP_DigestInit_ex (m_sha_ctx, EVP_sha512 (), nullptr);
    return {effective_size, std::move (sha)};
}

size_t smart_storage::free_space() {
    return std::min(m_size - m_used, uh::dbn::licensing::global_license_pointer_dbn->license_package()
        .free_count(uh::licensing::license_package::soft_metered_feature::LIMIT_STORAGE_CAPACITY););
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


} // end namespace uh::dbn::storage
