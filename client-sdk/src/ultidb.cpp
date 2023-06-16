/* Implementation of UltiDB APIs. */

#include "../include/ultidb.h"
#include <functional>
#include <thread>
#include <utility>
#include "protocol/client_pool.h"
#include "protocol/client_factory.h"
#include "net/plain_socket.h"
#include "chunking/mod.h"
#include "protocol/server.h"
#include <iostream>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// ---------------------------------------------------------------------

thread_local uint8_t error = 0;

// ---------------------------------------------------------------------

UDB_RESULT udb_get_last_error()
{
    return static_cast<UDB_RESULT>(error);
}

// ---------------------------------------------------------------------

const char* get_error_message()
{
    switch(error)
    {
        case 0 : return "Successful Operation.";
        case 2: return "Buffer Overflow";
        case 3: return "Undefined Chunking Mode";
        default: return "Unknown Error";
    }
}

// ---------------------------------------------------------------------

class udb_undefined : public std::exception
{
public:
    explicit udb_undefined(std::string  message) : message_(std::move(message)) {}

    [[nodiscard]] const char* what() const noexcept override
    {
        return message_.c_str();
    }

private:
    std::string message_;
};

// ---------------------------------------------------------------------

struct UDB_CONFIG_STRUCT
{
    UDB_CONFIG_STRUCT() :
            hostname(nullptr),
            port(0),
            connection_pool(3),
            chunking_mode(UDB_FAST_CDC)
    {};

    const char* hostname;
    uint16_t port;
    size_t connection_pool;
    UDB_CHUNKING_MODE chunking_mode;

};

// ---------------------------------------------------------------------

UDB_CONFIG* udb_create_config()
{
    try
    {
        return new UDB_CONFIG();
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
};

// ---------------------------------------------------------------------

UDB_RESULT udb_config_set_host_node(UDB_CONFIG* cfg, const char *hostname, uint16_t port,
                                         size_t connection_pool)
{
    try
    {
        cfg->hostname = hostname;
        cfg->port = port;
        cfg->connection_pool = connection_pool;
        return UDB_RESULT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_config_set_chunking_strategy(UDB_CONFIG* cfg, UDB_CHUNKING_MODE chunking_strategy)
{
    try
    {
        cfg->chunking_mode = chunking_strategy;
        return UDB_RESULT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_config(UDB_CONFIG *config)
{
    try
    {
        delete config;
        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

const char* get_sdk_version() { return SDK_VERSION; }

// ---------------------------------------------------------------------

UDB_KEY* udb_create_key(char* key_buffer, size_t length)
{
    try
    {
        return new UDB_KEY_STRUCT();
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_key(UDB_KEY* udb_key)
{
    try
    {
        delete udb_key;
        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_DOCUMENT* udb_create_document()
{
    try
    {
        return new UDB_DOCUMENT_STRUCT();
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_document(UDB_DOCUMENT* udb_doc)
{
    try
    {
        delete udb_doc;
        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

constexpr const char* Exception_Messsage(UDB_RESULT n)
{
    switch (n)
    {
        case UDB_RESULT::UDB_UNDEFINED_CHUNKING_MODE: return "Undefined chunking mode.";
    }

    throw std::logic_error("The given enum doesn't have any string associated with it.");
}

// ---------------------------------------------------------------------

constexpr const char* strategyString(UDB_CHUNKING_MODE n)
{
    switch (n)
    {
        case UDB_CHUNKING_MODE::UDB_FIXED_SIZE_CHUNK: return "FixedSize";
        case UDB_CHUNKING_MODE::UDB_RABIN_FINGERPRINT: return "CDCrabin";
        case UDB_CHUNKING_MODE::UDB_GEAR_CDC: return "Gear";
        case UDB_CHUNKING_MODE::UDB_FAST_CDC: return "FastCDC";
        case UDB_CHUNKING_MODE::UDB_MOD_CDC: return "ModCDC";
    }

    throw udb_undefined(Exception_Messsage(UDB_UNDEFINED_CHUNKING_MODE));
}

// ---------------------------------------------------------------------

struct UDB_STATE_STRUCT
{
    explicit UDB_STATE_STRUCT(UDB_CONFIG* config)
    {
        std::stringstream s;
        s << SDK_NAME << " " << SDK_VERSION;
        uh::protocol::client_factory_config cf_config
            {
                .client_version = s.str()
            };

        m_client_factory = std::make_unique<uh::protocol::client_factory>(
                std::make_unique<uh::net::plain_socket_factory>(m_io, config->hostname, config->port),
                cf_config);

    }

    std::unique_ptr<uh::chunking::mod> m_chunking_mod;
    boost::asio::io_context m_io;
    std::unique_ptr<uh::protocol::client_factory> m_client_factory;

};

// ---------------------------------------------------------------------

struct UDB_CONNECTION_STRUCT
{
    std::unique_ptr<uh::protocol::client> m_udb_client;

    explicit UDB_CONNECTION_STRUCT(UDB* handle)
    {
        m_udb_client = handle->m_client_factory->create();
    }
};

// ---------------------------------------------------------------------

UDB_CONNECTION* udb_connect(UDB* handle)
{
    try
    {
        return new UDB_CONNECTION_STRUCT(handle);
    }
    catch(std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB* udb_create_instance(UDB_CONFIG* config)
{
    try
    {
        return new UDB(config);
    }
    catch (const udb_undefined& e)
    {
        if ( std::string(e.what()) == Exception_Messsage(UDB_UNDEFINED_CHUNKING_MODE))
            error = UDB_UNDEFINED_CHUNKING_MODE;
        return nullptr;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_instance(UDB* ulti_db_instance)
{
    try
    {
        delete ulti_db_instance;
        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_ping(UDB_CONNECTION_STRUCT* conn)
{
    try
    {
        conn->m_udb_client->valid();
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_integrate(UDB_CONNECTION_STRUCT* conn, char* hash_buffer, size_t buffer_length, const char* data, size_t data_length)
{
    try
    {
        if (buffer_length < 65)
            throw std::overflow_error("Buffer overflow exception.");

        std::vector<uint32_t> chunk_sizes;
        chunk_sizes.push_back(static_cast<uint32_t>(data_length));
        auto resp = conn->m_udb_client->write_chunks(uh::protocol::write_chunks::request { chunk_sizes,
                                                                                      std::span<const char>(data, data_length) });

        std::memcpy(hash_buffer, resp.hashes.data(), 65);

        return UDB_RESULT::UDB_RESULT_SUCCESS;

    }
    catch(const std::overflow_error& e)
    {
        error = UDB_BUFFER_OVERFLOW;
        return UDB_BUFFER_OVERFLOW;
    }
    catch (const std::exception &e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_retrieve(UDB_CONNECTION_STRUCT* conn, char* buffer_to_fill, size_t buffer_length , const char* udb_hash)
{
    try
    {
        uh::protocol::read_chunks::request req { .hashes = std::span<const char>(udb_hash, 64)};
        auto result = conn->m_udb_client->read_chunks(req);

         /* BUG: Agency Node loses the chunk size information. */
         /* failure reading compression header should not be the error, instead couldn't read the hash should be the error */
//        auto retrieved_size = result.chunk_sizes.front();

        std::memcpy(buffer_to_fill, std::get<std::vector<char>>(result.data).data(), std::get<std::vector<char>>(result.data).size() );
    }
    catch(const std::overflow_error& e)
    {
        error = UDB_BUFFER_OVERFLOW;
        return UDB_BUFFER_OVERFLOW;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }

    return UDB_RESULT_SUCCESS;
}

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
