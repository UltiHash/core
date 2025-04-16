#include "service_id.h"

#include "common/etcd/namespace.h"
#include <fstream>

namespace uh::cluster {

namespace {

constexpr const char* IDENTITY_FILE_NAME = "identity";

std::pair<bool, std::size_t>
read_id_from_disk(const std::filesystem::path& id_file) {

    if (!std::filesystem::exists(id_file)) {
        return {false, 0};
    }

    std::ifstream in(id_file, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("could not read file " + id_file.string());
    }

    std::size_t persisted_id;
    in.read(reinterpret_cast<char*>(&persisted_id), sizeof(std::size_t));

    return {true, persisted_id};
}

void write_id_to_disk(const std::filesystem::path& id_file, std::size_t id) {

    std::filesystem::create_directories(id_file.parent_path());

    if (std::filesystem::exists(id_file)) {
        throw std::runtime_error(id_file.string() + " already exists");
    }

    std::ofstream out(id_file, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("could not open file " + id_file.string());
    }

    out.write(reinterpret_cast<const char*>(&id), sizeof(id));
}

} // namespace

std::size_t get_service_id(etcd_manager& etcd, const std::string& service,
                           const std::filesystem::path& data_dir) {

    auto id_file = data_dir / service / IDENTITY_FILE_NAME;

    auto [success, persisted_id] = read_id_from_disk(id_file);
    if (success) {
        return persisted_id;
    }

    std::string current_id_key = etcd_current_id_prefix_key + service;
    std::size_t current_id;

    const auto lock = etcd.get_lock_guard(etcd_global_lock_key);

    try {
        current_id = std::stoull(etcd.get(current_id_key));
    } catch (const std::exception&) {
        etcd.put(current_id_key, std::to_string(0));
        write_id_to_disk(id_file, 0);
        return 0;
    }

    current_id++;
    etcd.put(current_id_key, std::to_string(current_id));
    write_id_to_disk(id_file, current_id);
    return current_id;
}

std::size_t register_storage_service_id(etcd_manager& etcd,
                                        const std::size_t id) {
    std::string current_id_key =
        etcd_registered_storage_ids_prefix_key + std::to_string(id);

    const auto lock = etcd.get_lock_guard(etcd_storage_lock_key);

    std::size_t current_id = id;
    while (etcd.has(current_id_key)) {
        current_id++;
        current_id_key =
            etcd_registered_storage_ids_prefix_key + std::to_string(current_id);
    }

    current_id_key =
        etcd_registered_storage_ids_prefix_key + std::to_string(current_id);
    etcd.put(current_id_key, "1");
    return current_id;
}

} // namespace uh::cluster
