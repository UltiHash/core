#pragma once

#include <atomic>
#include <chrono>
#include <string>

namespace uh::cluster {

constexpr unsigned long long operator""_KiB(unsigned long long value) {
    return value * 1024;
}

constexpr unsigned long long operator""_MiB(unsigned long long value) {
    return value * 1024 * 1024;
}

constexpr unsigned long long operator""_GiB(unsigned long long value) {
    return value * 1024 * 1024 * 1024;
}

constexpr unsigned long long operator""_TiB(unsigned long long value) {
    return value * 1024 * 1024 * 1024 * 1024;
}

constexpr unsigned long long operator""_PiB(unsigned long long value) {
    return value * 1024 * 1024 * 1024 * 1024 * 1024;
}

static constexpr std::size_t KIBI_BYTE = 1024;
static constexpr std::size_t MEBI_BYTE = 1024 * KIBI_BYTE;
static constexpr std::size_t GIBI_BYTE = 1024 * MEBI_BYTE;
static constexpr std::size_t TEBI_BYTE = 1024 * GIBI_BYTE;
static constexpr std::size_t PEBI_BYTE = 1024 * TEBI_BYTE;

enum role : uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATOR_SERVICE,
    ENTRYPOINT_SERVICE,
    COORDINATOR_SERVICE
};

inline role global_service_role;

enum message_type : uint8_t {

    SUCCESS = 0,
    FAILURE = 1,

    STORAGE_READ_ADDRESS_REQ = 33,
    STORAGE_READ_REQ = 34,
    STORAGE_WRITE_REQ = 35,
    STORAGE_LINK_REQ = 37,
    STORAGE_UNLINK_REQ = 38,
    STORAGE_USED_REQ = 39,
    STORAGE_ALLOCATE_REQ = 40,

    DEDUPLICATOR_REQ = 64,

};

constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
constexpr const char* UH_WORKING_DIR = "UH_WORKING_DIR";
constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";
constexpr const char* ENV_CFG_LICENSE = "UH_LICENSE";
constexpr const char* ENV_CFG_STORAGE_GROUPS = "UH_STORAGE_GROUPS";
constexpr const char* ENV_CFG_BACKEND_HOST = "UH_BACKEND_HOST";
constexpr const char* ENV_CFG_CUSTOMER_ID = "UH_CUSTOMER_ID";
constexpr const char* ENV_CFG_ACCESS_TOKEN = "UH_ACCESS_TOKEN";
constexpr const char* ENV_CFG_OTEL_ENDPOINT = "UH_OTEL_ENDPOINT";
constexpr const char* ENV_CFG_OTEL_EXPORT_INTERVAL = "UH_OTEL_INTERVAL";
constexpr const char* ENV_CFG_ENABLE_TRACES = "UH_TRACES_ENABLED";
constexpr const char* ENV_CFG_DB_HOSTPORT = "UH_DB_HOSTPORT";
constexpr const char* ENV_CFG_DB_DIRECTORY_CONNECTIONS =
    "UH_DB_DIRECTORY_CONNECTIONS";
constexpr const char* ENV_CFG_DB_MULTIPART_CONNECTIONS =
    "UH_DB_MULTIPART_CONNECTIONS";
constexpr const char* ENV_CFG_DB_USERS_CONNECTIONS = "UH_DB_USERS_CONNECTIONS";
constexpr const char* ENV_CFG_DB_USER = "UH_DB_USER";
constexpr const char* ENV_CFG_DB_PASS = "UH_DB_PASS";
constexpr const char* ENV_CFG_ETCD_USERNAME = "UH_ETCD_USERNAME";
constexpr const char* ENV_CFG_ETCD_PASSWORD = "UH_ETCD_PASSWORD";
constexpr const char* ENV_CFG_NO_DEDUPE = "UH_NO_DEDUPE";
constexpr const char* ENV_CFG_STORAGE_SERVICE_ID = "UH_STORAGE_INSTANCE_ID";
constexpr const char* ENV_CFG_STORAGE_GROUP_ID = "UH_STORAGE_GROUP_ID";

constexpr const char* RESERVED_BUCKET_NAME = "ultihash";

class time_settings {
public:
    using duration_t = std::chrono::steady_clock::duration;
    static time_settings& instance() {
        static time_settings inst;
        return inst;
    }
    auto get_service_get_timeout() const {
        return service_get_timeout.load(std::memory_order_acquire);
    }
    auto get_group_state_wait_timeout() const {
        return group_state_wait_timeout.load(std::memory_order_acquire);
    }
    auto get_offset_gathering_timeout() const {
        return offset_gathering_timeout.load(std::memory_order_acquire);
    }
    auto get_async_io_timeout() const {
        return async_io_timeout.load(std::memory_order_acquire);
    }
    auto get_license_fetch_period() const {
        return license_fetch_period.load(std::memory_order_acquire);
    }

    void set_service_get_timeout(duration_t timeout) {
        service_get_timeout.store(timeout, std::memory_order_release);
    }
    void set_group_state_wait_timeout(duration_t timeout) {
        group_state_wait_timeout.store(timeout, std::memory_order_release);
    }
    void set_offset_gathering_timeout(duration_t timeout) {
        offset_gathering_timeout.store(timeout, std::memory_order_release);
    }
    void set_async_io_timeout(duration_t timeout) {
        async_io_timeout.store(timeout, std::memory_order_release);
    }
    void set_license_fetch_period(duration_t timeout) {
        license_fetch_period.store(timeout, std::memory_order_release);
    }

private:
    std::atomic<duration_t> service_get_timeout{std::chrono::seconds(10)};
    std::atomic<duration_t> group_state_wait_timeout{std::chrono::seconds(10)};
    std::atomic<duration_t> offset_gathering_timeout{std::chrono::seconds(2)};
    std::atomic<duration_t> async_io_timeout{std::chrono::seconds(30)};
    std::atomic<duration_t> license_fetch_period{std::chrono::hours(1)};
};

constexpr std::string_view CONFIG_PATH_DELIMETER = ":";

constexpr size_t SET_LOG_CACHE_SIZE = 10000;
constexpr size_t INPUT_CHUNK_SIZE = 64ul * MEBI_BYTE;

constexpr std::size_t DEFAULT_PAGE_SIZE = 8 * KIBI_BYTE;

const std::string& get_service_string(const role& service_role);

} // end namespace uh::cluster
