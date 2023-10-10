////
//// Created by max on 26.05.23.
//// inspired by https://github.com/FreeYourSoul/free_fdb
////
//
//#ifndef CORE_FDB_H
//#define CORE_FDB_H
////#include <fmt/format.h>
//
//#include <util/exception.h>
//#include <logging/logging_boost.h>
//
//#include <span>
//#include <optional>
//
//#define FDB_API_VERSION 710
//#include <foundationdb/fdb_c.h>
//
//namespace uh::fdb {
//
//    DEFINE_EXCEPTION(fdb_exception);
//    DEFINE_EXCEPTION(fdb_transaction_exception);
//
//    static void check_fdb_code(fdb_error_t error) {
//        if (error != 0) {
//            if (fdb_error_predicate(FDBErrorPredicate::FDB_ERROR_PREDICATE_RETRYABLE, error)) {
//                THROW(fdb_transaction_exception, "Future, Non retry-able error : " + std::string(fdb_get_error(error)));
//            }
//            THROW(fdb_exception, "Future, Other error : " + std::string(fdb_get_error(error)));
//        }
//    }
//
//    struct fdb_result {
//        std::vector<char> key;
//        std::vector<char> value;
//    };
//
//    class fdb_future {
//    public:
//        ~fdb_future() {
//            if (_data) {
//                fdb_future_destroy(_data);
//            }
//        }
//        fdb_future(const fdb_future &) = delete;
//
//        explicit fdb_future(FDBFuture *fut) : _data(fut) {
//        }
//
//        template<typename Handler>
//        auto get(Handler &&handler) {
//            if (!_data) {
//                THROW(fdb_exception, "Error: Future data is null and thus cant be awaited.");
//            }
//            if (auto error = fdb_future_block_until_ready(_data); error != 0) {
//                THROW(fdb_exception, "Error on future block : " + std::string(fdb_get_error(error)));
//            }
//            check_fdb_code(fdb_future_get_error(_data));
//            return std::forward<Handler>(handler)(_data);
//        }
//
//        void get() {
//            get([](FDB_future *) { return std::optional<fdb_result>{}; });
//        }
//
//    private:
//        FDBFuture *_data;
//    };
//
///**
// * @brief RAII object encapsulating a FDBTransaction
// * If not committed, transaction is rolled back at destruction time.
// *
// * @see https://apple.github.io/foundationdb/api-c.html#transaction
// */
//    class fdb_transaction {
//
//    public:
//        ~fdb_transaction();
//        fdb_transaction(const fdb_transaction &) = delete;
//        explicit fdb_transaction(FDBDatabase *db);
//
//        /**
//         * @brief Enable snapshot
//         * @see https://apple.github.io/foundationdb/api-c.html#snapshot-reads
//         */
//        void enable_snapshot();
//
//        /**
//         * @brief Commit the current transaction
//         *
//         * @see https://apple.github.io/foundationdb/api-c.html#c.fdb_transaction_commit
//         */
//        void commit();
//
//        /**
//         * @brief Reset the current transaction to its original state
//         *
//         * @see https://apple.github.io/foundationdb/api-c.html#c.fdb_transaction_reset
//         */
//        void reset();
//
//        /**
//         * @brief Insert a new key / value pair in foundationdb
//         *
//         * @param key to insert
//         * @param value to insert
//         */
//        void put(const std::span<char> &key, const std::span<char> &value);
//
//        /**
//         * @brief Remove a selected key from the DB
//         * @param key to delete in foundationdb
//         */
//        void del(const std::span<char> &key);
//
//        /**
//         * @warning this method is for internal purpose only and should not be used in order to improvise C API calls
//         * @return the raw C API foundationdb transaction encapsulated in the current transaction
//         */
//        [[nodiscard]] FDBTransaction *raw() const;
//
//        /**
//         * @brief Get the key value at the specified key in foundationdb
//         *
//         * @param key to retrieve from the database
//         * @return a key value structure if present, std::nullopt otherwise
//         */
//        std::optional<fdb_result> get(const std::span<char> &key);
//
//    private:
//        FDBTransaction *_trans = nullptr;
//        bool _snapshot_enabled = false;
//    };
//
///**
// * @brief RAII Object representing an instance of the foundationdb,
// * At construction time a thread is launched in order to handle the fdb network.
// * At the first initialization of the foundationdb (ensured by a std::once_flag semaphore) network is setup
// *
// * Encapsulate a FDBDatabase pointer, connection to the database and network thread is closed when the object
// * is destructed
// *
// * @see https://apple.github.io/foundationdb/api-c.html#database
// * @see https://apple.github.io/foundationdb/api-c.html#network
// */
//    class fdb {
//        struct internal;
//
//    public:
//        ~fdb();
//        explicit fdb(const std::string &cluster_file_path);
//
//        /**
//         * @return a pointer on a newly created transaction raii object
//         */
//        [[nodiscard]] std::unique_ptr<fdb_transaction> make_transaction();
//
//
//    private:
//        std::unique_ptr<internal> _impl;
//    };
//
//}// namespace uh::fdb
//
//#endif //CORE_FDB_H
