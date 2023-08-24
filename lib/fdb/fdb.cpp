//
// Created by max on 26.05.23.
//

#include "fdb.h"

#include <mutex>
#include <thread>

//#include <internal/future.hh>


namespace uh::fdb {

    static std::once_flag version_select_flag;

    constexpr fdb_bool_t not_reversed() {
        return fdb_bool_t{0};
    }

    struct fdb::internal {

        explicit internal(const std::string &cluster_file_path) {

            std::call_once(version_select_flag, [this]() {
                if (auto error = fdb_select_api_version(FDB_API_VERSION); error) {
                    THROW(fdb_exception, "Error Selecting version: " + std::string(fdb_get_error(error)));
                }

                // Option debug trace
                // fdb_network_set_option(FDBNetworkOption::FDB_NET_OPTION_TRACE_ENABLE, ...)

                if (auto error = fdb_setup_network(); error) {
                    THROW(fdb_exception, "Error setup network: " + std::string(fdb_get_error(error)));
                }

                t = std::thread([]() {
                    if (auto error = fdb_run_network(); error) {
                        THROW(fdb_exception, "Error while starting network: " + std::string(fdb_get_error(error)));
                    }
                });
            });

            if (auto error = fdb_create_database(cluster_file_path.c_str(), &db); error) {
                THROW(fdb_exception, "Error creating DB: " + std::string(fdb_get_error(error)));
            }
        }

        ~internal() {
            auto err = fdb_stop_network();
            if (err) {
                /* An error occurred (probably network not running) */
                WARNING << "Error while stopping fdb network";
            }
            if (t.joinable()) {
                t.join();
            }
        }

        FDBDatabase *db{};

        std::thread t;
    };

    fdb::~fdb() {
        if (_impl->db) {
            fdb_database_destroy(_impl->db);
        }
    }

    fdb::fdb(const std::string &cluster_file_path) : _impl(std::make_unique<internal>(cluster_file_path)) {
    }

    std::unique_ptr<fdb_transaction> fdb::make_transaction() {
        return std::make_unique<fdb_transaction>(_impl->db);
    }

    fdb_transaction::fdb_transaction(FDBDatabase *db) {
        check_fdb_code(fdb_database_create_transaction(db, &_trans));
    }

    fdb_transaction::~fdb_transaction() {
        if (_trans) {
            fdb_transaction_destroy(_trans);
        }
    }

    void fdb_transaction::put(const std::span<char> &key, const std::span<char> &value) {
        if (_trans) {
            const auto *key_name = reinterpret_cast<const uint8_t *>(key.data());
            const auto *value_name = reinterpret_cast<const uint8_t *>(value.data());
            fdb_transaction_set(_trans, key_name, key.size(), value_name, value.size());
        }
    }

    void fdb_transaction::del(const std::span<char> &key) {
        if (_trans) {
            fdb_transaction_clear(_trans, reinterpret_cast<const uint8_t *>(key.data()), key.size());
        }
    }


    std::optional<fdb_result> fdb_transaction::get(const std::span<char> &key) {
        if (_trans) {
            const auto *key_name = reinterpret_cast<const uint8_t *>(key.data());
            auto fut = fdb_future(fdb_transaction_get(_trans, key_name, key.size(), _snapshot_enabled));

            return fut.get([&key](FDBFuture *f) -> std::optional<fdb_result> {
                fdb_bool_t out_present;
                const uint8_t *out_value;
                int out_length;

                check_fdb_code(fdb_future_get_value(f, &out_present, &out_value, &out_length));
                if (!out_present) {
                    return std::nullopt;
                }

                return fdb_result{
                        std::vector<char>(key.begin(), key.end()),
                        std::vector<char>(reinterpret_cast<const char *>(out_value), reinterpret_cast<const char *>(out_value) + out_length)
                        };
            });
        }
        return std::nullopt;
    }

    void fdb_transaction::enable_snapshot() {
        _snapshot_enabled = true;
    }

    FDBTransaction *fdb_transaction::raw() const {
        return _trans;
    }

    void fdb_transaction::reset() {
        fdb_transaction_reset(_trans);
    }

    void fdb_transaction::commit() {
        fdb_future(fdb_transaction_commit(_trans)).get();
    }

}// namespace uh::fdb
