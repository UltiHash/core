#include <iostream>
#include <sqlite3.h>
#include <cstring>
#include <vector>
#include <chrono>
#include <openssl/sha.h>

static constexpr std::size_t KIBI_BYTE = 1024;
static constexpr std::size_t MEBI_BYTE = 1024 * KIBI_BYTE;
static constexpr std::size_t GIBI_BYTE = 1024 * MEBI_BYTE;

// Function to generate random data for hash, address, and metadata
void generateRandomData(char* buffer, int size) {
}

void computeSHA256(const unsigned char* data, size_t length, unsigned char* hash) {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, length);
    SHA256_Final(hash, &sha256);
}

int main() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("/data/test.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    // Create table
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS entries (hash BLOB(32), address BIGINT, metadata BIGINT);";
    rc = sqlite3_exec(db, createTableSQL, nullptr, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }

    // Create index on hash column
    const char* createIndexSQL = "CREATE INDEX IF NOT EXISTS hash_index ON entries(hash);";
    rc = sqlite3_exec(db, createIndexSQL, nullptr, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return 1;
    }

    // Prepare SQL statement for inserting data
    sqlite3_stmt *stmt;
    const char* insertSQL = "INSERT INTO entries (hash, address, metadata) VALUES (?, ?, ?);";
    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    // Prepare SQL statement for reading data
    sqlite3_stmt *readStmt;
    const char* readSQL = "SELECT address FROM entries WHERE hash = ?;";
    rc = sqlite3_prepare_v2(db, readSQL, -1, &readStmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare read statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return 1;
    }

    const uint64_t dataVolume = 64 * GIBI_BYTE;
    const uint64_t numEntries = dataVolume / (8 * KIBI_BYTE);

    std::cout << "Number of entries: " << numEntries << ", requiring at least " << numEntries * 48 / MEBI_BYTE << "MiB" << std::endl;

    auto start = std::chrono::steady_clock::now();
    // Begin transaction
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    for (uint64_t i = 0; i < numEntries; i++) {
        unsigned char sha256Hash[SHA256_DIGEST_LENGTH];
        computeSHA256(reinterpret_cast<const unsigned char*>(&i), sizeof(i), sha256Hash);
        uint64_t address = rand();
        uint64_t metadata = rand();



        sqlite3_bind_blob(stmt, 1, sha256Hash, 32, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, address);
        sqlite3_bind_int64(stmt, 3, metadata);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Error inserting data: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        sqlite3_reset(stmt);
    }
    // Commit transaction
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    double insertsPerSecond = static_cast<double>(numEntries) / (static_cast<double>(duration) / 1000.0);
    std::cout << "Inserted " << numEntries << " entries in " << duration << " milliseconds. Inserts per second: " << insertsPerSecond << std::endl;
    std::cout << "This is equivalent to an raw data ingestion rate of " << (insertsPerSecond * (8 * KIBI_BYTE)) / MEBI_BYTE << " MiB/s" << std::endl;

    start = std::chrono::steady_clock::now();
    for (uint64_t i = 0; i < numEntries; i++) {
        unsigned char sha256Hash[SHA256_DIGEST_LENGTH];
        computeSHA256(reinterpret_cast<const unsigned char*>(&i), sizeof(i), sha256Hash);

        sqlite3_bind_blob(readStmt, 1, sha256Hash, SHA256_DIGEST_LENGTH, SQLITE_TRANSIENT);

        while (sqlite3_step(readStmt) == SQLITE_ROW) {
            // Read the data
            // For now, just retrieving the data and ignoring it
        }

        sqlite3_reset(readStmt);
    }

    duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    double readsPerSecond = static_cast<double>(numEntries) / (static_cast<double>(duration) / 1000.0);

    std::cout << "Read " << numEntries << " entries in " << duration << " milliseconds. Reads per second: " << readsPerSecond << std::endl;
    std::cout << "This is equivalent to an raw data egress rate of " << (readsPerSecond * (8 * KIBI_BYTE)) / MEBI_BYTE << " MiB/s" << std::endl;


    // Finalize the statement and close the database
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}

