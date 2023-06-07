/* Implementation of UltiDB APIs. */

#include "../include/ultidb.h"
#include <functional>
#include <thread>
#include <protocol/client_pool.h>
#include <protocol/client_factory.h>
#include <net/plain_socket.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// ---------------------------------------------------------------------

thread_local uint8_t error = 0;

// ---------------------------------------------------------------------

class Buffer_Overflow : public std::exception
{
public:
    [[nodiscard]] const char* what() const noexcept override
    {
        return "Buffer overflow exception";
    }
};

// ---------------------------------------------------------------------

ULTIDB_RESULT ultidb_get_last_error()
{
    return static_cast<ULTIDB_RESULT>(error);
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

};

// ---------------------------------------------------------------------

ULTIDB_CONFIG* udb_create_config()
{
    try
    {
        return new ULTIDB_CONFIG();
    }
    catch (const std::exception& e)
    {
        error = ULTIDB_RESULT_ERROR;
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
        error = ULTIDB_RESULT_ERROR;
        return ULTIDB_FALSE;
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
        error = ULTIDB_RESULT_ERROR;
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
        error = ULTIDB_RESULT_ERROR;
        return ULTIDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

const char* get_sdk_version() { return PROJECT_VERSION; }

// ---------------------------------------------------------------------

struct ULTIDB_STATE_STRUCT
{
    explicit ULTIDB_STATE_STRUCT(ULTIDB_CONFIG* config)
    {
        boost::asio::io_context io;
        std::stringstream s;
        s << SDK_NAME << " " << SDK_VERSION;
        uh::protocol::client_factory_config cf_config
                {
                        .client_version = s.str()
                };

        connection_pool = std::make_unique<uh::protocol::client_pool>(std::make_unique<uh::protocol::client_factory>(
                std::make_unique<uh::net::plain_socket_factory>(io, config->hostname, config->port),
                cf_config), config->connection_pool);

    }

    std::unique_ptr<uh::protocol::client_pool> connection_pool;
};

// ---------------------------------------------------------------------

ULTIDB* udb_create_instance(ULTIDB_CONFIG* config)
{
    try
    {
        return new ULTIDB(config);
    }
    catch (const std::exception& e)
    {
        error = ULTIDB_RESULT_ERROR;
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
        error = ULTIDB_RESULT_ERROR;
        return ULTIDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

ULTIDB_RESULT udb_integrate(ULTIDB *db, char* hash_buffer, const char* data, size_t length)
{
    try
    {
        // divide into chunks here using the strategy
        uh::protocol::write_chunks::request req { .chunk_sizes = std::span<uint32_t>(), .data = std::span<char>() };
        auto result = db->connection_pool->get()->write_chunks(req);

        // write hash in a buffer given
    }
    catch (const std::exception &e)
    {
        error = ULTIDB_RESULT_ERROR;
        return ULTIDB_RESULT::ULTIDB_RESULT_ERROR;
    }

    return ULTIDB_RESULT::ULTIDB_RESULT_SUCCESS;
}

// ---------------------------------------------------------------------

ULTIDB_RESULT udb_retrieve(ULTIDB *db, char* buffer_to_fill, size_t buffer_length ,const char* udb_hash, size_t hash_length, uint32_t* filled_length)
{
    try
    {
        uh::protocol::read_chunks::request req { .hashes = std::span<const char>(udb_hash, hash_length)};
        auto result = db->connection_pool->get()->read_chunks(req);

        auto retrieved_size = result.chunk_sizes.front();
        *filled_length = retrieved_size;

        if (retrieved_size > buffer_length)
            throw Buffer_Overflow();

        std::memcpy(buffer_to_fill, std::get<std::vector<char>>(result.data).data(), result.chunk_sizes.front());
    }
    catch(const Buffer_Overflow& e)
    {
        error = ULTIDB_BUFFER_OVERFLOW;
        return ULTIDB_BUFFER_OVERFLOW;
    }
    catch (const std::exception& e)
    {
        error = ULTIDB_RESULT_ERROR;
        return ULTIDB_RESULT_ERROR;
    }

    return ULTIDB_RESULT_SUCCESS;
}

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
