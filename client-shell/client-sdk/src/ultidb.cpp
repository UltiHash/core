/* Implementation of UltiDB APIs. */

#include "../include/ultidb.h"
#include <functional>
#include <thread>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// ---------------------------------------------------------------------

struct ULTIDB_STATE_STRUCT
{
    explicit ULTIDB_STATE_STRUCT(ULTIDB_CONFIG *config)
    {

    }

    std::function<ULTIDB_RESULT(const char *chunk, size_t length)> write_chunks{};
    std::function<char* (const char *hash)> read_chunks{};

    /* error number is set when an error occurs inside ULTIDB_STATE_STRUCT */
    thread_local static uint8_t error;
};

// ---------------------------------------------------------------------

thread_local uint8_t ULTIDB_STATE_STRUCT::error = 0;

// ---------------------------------------------------------------------

/* Why do we even need errno since after every operation we send the error code regarless as defined in
 * header file */
uint8_t udb_get_errno(ULTIDB *db)
{
    return db->error;
}

// ---------------------------------------------------------------------

struct ULTIDB_CONFIG_STRUCT
{
    ULTIDB_CONFIG_STRUCT() :
    hostname(nullptr),
    port(0),
    connection_pool(3),
    chunking_mode(ULTIDB_FAST_CDC)
    {};

    const char* hostname;
    uint16_t port;
    size_t connection_pool;
    ULTIDB_CHUNKING_MODE chunking_mode;

    /* error number is set when an error occurs inside ULTIDB_CONFIG_STRUCT */
    thread_local static uint8_t error;
};

// ---------------------------------------------------------------------

thread_local uint8_t ULTIDB_CONFIG_STRUCT::error = 0;

// ---------------------------------------------------------------------

ULTIDB_CONFIG* udb_create_config()
{
    try
    {
        return new ULTIDB_CONFIG();
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
};

// ---------------------------------------------------------------------

ULTIDB_BOOL udb_config_set_agencynode(ULTIDB_CONFIG* cfg, const char *hostname, uint16_t port,
                                         size_t connection_pool)
{
    try
    {
        cfg->hostname = hostname;
        cfg->port = port;
        cfg->connection_pool = connection_pool;
        return ULTIDB_TRUE;
    }
    catch (const std::exception& e)
    {
        return ULTIDB_FALSE;
    }
}

// ---------------------------------------------------------------------

ULTIDB_RESULT udb_destroy_config(ULTIDB_CONFIG *config)
{
    try
    {
        delete config;
        return ULTIDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        return ULTIDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

ULTIDB_BOOL udb_config_set_chunking_strategy(ULTIDB_CONFIG* cfg, ULTIDB_CHUNKING_MODE chunking_strategy)
{
    try
    {
        cfg->chunking_mode = chunking_strategy;
        return ULTIDB_TRUE;
    }
    catch (const std::exception& e)
    {
        return ULTIDB_FALSE;
    }
}

// ---------------------------------------------------------------------

ULTIDB* udb_create_instance(ULTIDB_CONFIG* config)
{
    try
    {
        return new ULTIDB(config);
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

// ---------------------------------------------------------------------

ULTIDB_RESULT udb_destroy_instance(ULTIDB* ulti_db_instance)
{
    try
    {
        delete ulti_db_instance;
        return ULTIDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        return ULTIDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

ULTIDB_RESULT udb_integrate(ULTIDB *db, const char* data)
{
    try
    {
        // divide into chunks here using the strategy
        std::string chunk{};
        size_t length = 0;
        auto result = db->write_chunks(chunk.c_str(), length);
    }
    catch (const std::exception &e)
    {
        return ULTIDB_RESULT::ULTIDB_RESULT_ERROR;
    }

    return ULTIDB_RESULT::ULTIDB_RESULT_SUCCESS;
}

// ---------------------------------------------------------------------

char* udb_retrieve(ULTIDB *db, const char* udb_hash)
{
    try
    {
        auto result = db->read_chunks(udb_hash);
        return result;
    }
    catch (const std::exception& e)
    {
        return nullptr;
    }
}

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
