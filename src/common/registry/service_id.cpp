#include "service_id.h"

#include "common/registry/namespace.h"
#include "common/utils/common.h"
#include "common/utils/time_utils.h"
#include <fstream>

namespace uh::cluster {

namespace {

static constexpr const char* IDENTITY_FILE_NAME = "identity";

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

std::string get(etcd::SyncClient& client, const std::string& key) {

    const auto response = wait_for_success(ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
                                           [&]() { return client.get(key); });

    if (response.is_ok())
        return response.value().as_string();
    else
        throw std::invalid_argument(
            "retrieval of configuration parameter " + key +
            " failed, details: " + response.error_message());
}

std::string set(etcd::SyncClient& client, const std::string& key,
                const std::string& value) {

    const auto response =
        wait_for_success(ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
                         [&]() { return client.set(key, value); });

    if (response.is_ok())
        return response.value().as_string();
    else
        throw std::invalid_argument(
            "setting the configuration parameter " + key +
            " failed, details: " + response.error_message());
}

class registry_lock {
public:
    explicit registry_lock(etcd::SyncClient& client)
        : m_client(client),
          m_response(m_client.lock(etcd_global_lock_key)) {}

    ~registry_lock() { m_client.unlock(m_response.lock_key()); }

private:
    etcd::SyncClient& m_client;
    etcd::Response m_response;
};

} // namespace

std::size_t get_service_id(etcd::SyncClient& client, const std::string& service,
                           const std::filesystem::path& data_dir) {

    auto id_file = data_dir / service / IDENTITY_FILE_NAME;

    auto [success, persisted_id] = read_id_from_disk(id_file);
    if (success) {
        return persisted_id;
    }

    std::string current_id_key = etcd_current_id_prefix_key + service;
    std::size_t current_id;

    const auto lock = wait_for_success(ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
                                       [&]() { return registry_lock(client); });

    try {
        current_id = std::stoull(get(client, current_id_key));
    } catch (const std::exception&) {
        set(client, current_id_key, std::to_string(0));
        write_id_to_disk(id_file, 0);
        return 0;
    }

    current_id++;
    set(client, current_id_key, std::to_string(current_id));
    write_id_to_disk(id_file, current_id);
    return current_id;
}

} // namespace uh::cluster
