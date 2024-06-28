#include "common/utils/random.h"

#include <chrono>
#include <iostream>
#include <openssl/sha.h>
#include <random>
#include <rocksdb/db.h>
#include <thread>
#include <vector>

static constexpr std::size_t KIBI_BYTE = 1024;
static constexpr std::size_t MEBI_BYTE = 1024 * KIBI_BYTE;
static constexpr std::size_t GIBI_BYTE = 1024 * MEBI_BYTE;
static constexpr std::size_t TEBI_BYTE = 1024 * GIBI_BYTE;

// Function to compute SHA-256 hash
void computeSHA256(const unsigned char* data, size_t length,
                   unsigned char* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, length);
    SHA256_Final(hash, &sha256);
}

void put_function(rocksdb::DB* db, uint64_t start, uint64_t end) {
    for (uint64_t i = start; i < end; ++i) {
        unsigned char sha256Hash[SHA256_DIGEST_LENGTH];
        computeSHA256(reinterpret_cast<const unsigned char*>(&i), sizeof(i),
                      sha256Hash);
        std::string key(reinterpret_cast<const char*>(sha256Hash),
                        SHA256_DIGEST_LENGTH);

        std::string value = uh::cluster::random_string(16);

        db->Put(rocksdb::WriteOptions(), key, value);
    }
}

void get_function(rocksdb::DB* db, uint64_t start, uint64_t end) {
    for (uint64_t i = start; i < end; ++i) {
        unsigned char sha256Hash[SHA256_DIGEST_LENGTH];
        computeSHA256(reinterpret_cast<const unsigned char*>(&i), sizeof(i),
                      sha256Hash);
        std::string key(reinterpret_cast<const char*>(sha256Hash),
                        SHA256_DIGEST_LENGTH);

        std::string value;
        db->Get(rocksdb::ReadOptions(), key, &value);
    }
}

int main() {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeForPointLookup(16384);
    options.create_if_missing = true;

    rocksdb::Status status =
        rocksdb::DB::Open(options, "/data/rocksdb_test", &db);
    if (!status.ok()) {
        std::cerr << "Unable to open database: " << status.ToString()
                  << std::endl;
        return 1;
    }

    const uint64_t dataVolume = 1 * TEBI_BYTE;
    const uint64_t numEntries = dataVolume / (8 * KIBI_BYTE);
    const uint64_t numThreads = 8;
    const uint64_t entriesPerThread = numEntries / numThreads;

    std::vector<std::thread> put_threads;

    std::cout << "Number of entries: " << numEntries << ", requiring at least "
              << numEntries * 48 / MEBI_BYTE << "MiB" << std::endl;

    auto start = std::chrono::steady_clock::now();
    for (uint64_t i = 0; i < numThreads; ++i) {
        uint64_t index_start = i * entriesPerThread;
        uint64_t index_end = (i + 1) * entriesPerThread;
        put_threads.emplace_back(
            put_function, db, index_start,
            index_end); // Passes the thread ID to the function
    }

    for (auto& thread : put_threads) {
        thread.join();
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count();
    double insertsPerSecond = static_cast<double>(numEntries) /
                              (static_cast<double>(duration) / 1000.0);

    std::cout << "Inserted " << numEntries << " entries " << duration
              << " milliseconds. Inserts per second: " << insertsPerSecond
              << std::endl;
    std::cout << "This is equivalent to an raw data ingestion rate of "
              << (insertsPerSecond * (8 * KIBI_BYTE)) / MEBI_BYTE << " MiB/s"
              << std::endl;

    std::vector<std::thread> get_threads;
    start = std::chrono::steady_clock::now();
    for (uint64_t i = 0; i < numThreads; ++i) {
        uint64_t index_start = i * entriesPerThread;
        uint64_t index_end = (i + 1) * entriesPerThread;
        get_threads.emplace_back(
            get_function, db, index_start,
            index_end); // Passes the thread ID to the function
    }

    for (auto& thread : get_threads) {
        thread.join();
    }

    duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start)
                   .count();
    double readsPerSecond = static_cast<double>(numEntries) /
                            (static_cast<double>(duration) / 1000.0);

    std::cout << "Read " << numEntries << " entries " << duration
              << " milliseconds. Reads per second: " << readsPerSecond
              << std::endl;
    std::cout << "This is equivalent to an raw data egress rate of "
              << (readsPerSecond * (8 * KIBI_BYTE)) / MEBI_BYTE << " MiB/s"
              << std::endl;

    delete db;

    return 0;
}
