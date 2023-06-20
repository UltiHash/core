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
            connection_pool(3)
    {};

    const char* hostname;
    uint16_t port;
    size_t connection_pool;

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

UDB_KEY* udb_create_key(char* key_buffer, size_t size)
{
    try
    {
        return new UDB_KEY_STRUCT(key_buffer, size);
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_KEY* udb_create_empty_key()
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
        delete udb_key->key;
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

UDB_RESULT udb_destroy_multiple_keys(const UDB_KEY** key_container, size_t size)
{
    try
    {
        while (size > 0)
        {
            --size;
            delete key_container[size]->key;
            delete key_container[size];
        }

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_DOCUMENT* udb_create_document(char* data, size_t size)
{
    try
    {
        return new UDB_DOCUMENT_STRUCT(data, size);
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_DOCUMENT* udb_create_empty_document()
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

void udb_document_set_data(UDB_DOCUMENT* doc, char* data, size_t size)
{
    doc->data = data;
    doc->size = size;
}

// ---------------------------------------------------------------------

void udb_document_get_data(UDB_DOCUMENT* doc, char** data, size_t* size)
{
    *data = doc->data;
    *size = doc->size;
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_document(UDB_DOCUMENT* udb_doc)
{
    try
    {
        delete udb_doc->data;
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

UDB_RESULT udb_destroy_multiple_documents(const UDB_DOCUMENT** doc_container, size_t size)
{
    try
    {
        while (size > 0)
        {
            --size;
            delete doc_container[size]->data;
            delete doc_container[size];
        }

        return UDB_RESULT::UDB_RESULT_SUCCESS;
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
//        case UDB_RESULT::UDB_UNDEFINED_CHUNKING_MODE: return "Undefined chunking mode.";
        default: return "unrecognized error.";
    }

    throw std::logic_error("The given enum doesn't have any string associated with it.");
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

    boost::asio::io_context m_io;
    std::unique_ptr<uh::protocol::client_factory> m_client_factory;

};

// ---------------------------------------------------------------------

struct UDB_CONNECTION_STRUCT
{
    std::unique_ptr<uh::protocol::client> m_udb_client;

    explicit UDB_CONNECTION_STRUCT(UDB* udb)
    {
        m_udb_client = udb->m_client_factory->create();
    }
};

// ---------------------------------------------------------------------

UDB_CONNECTION* udb_create_connection(UDB* instance)
{
    try
    {
        return new UDB_CONNECTION_STRUCT(instance);
    }
    catch(std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_connection(UDB_CONNECTION* conn)
{
    try
    {
        delete conn;
        return UDB_RESULT_SUCCESS;
    }
    catch(std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB* udb_create_instance(UDB_CONFIG* config)
{
    try
    {
        return new UDB(config);
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_instance(UDB* instance)
{
    try
    {
        delete instance;
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
        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_add(UDB_CONNECTION* conn,
                   const UDB_DOCUMENT** docs,
                   UDB_KEY** key,
                   size_t n)
{
    try
    {
        for (size_t index = 0; index < n; index++)
        {
            std::vector<uint32_t> chunk_sizes; // TODO: inefficient has to change
            chunk_sizes.push_back(static_cast<uint32_t>(docs[index]->size));

            auto resp = conn->m_udb_client->write_chunks(uh::protocol::write_chunks::request
                    { chunk_sizes, std::span<const char>(docs[index]->data, docs[index]->size ) });

            char* returned_key = new char[64]{};
            std::memcpy(returned_key, resp.hashes.data(), 64); // TODO: inefficient copy

            key[index] = new UDB_KEY(returned_key, 64);
        }

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_add_one(UDB_CONNECTION* conn,
                       const UDB_DOCUMENT* doc,
                       UDB_KEY* key)
{
    try
    {
        std::vector<uint32_t> chunk_sizes; // TODO: inefficient has to change
        chunk_sizes.push_back(static_cast<uint32_t>(doc->size));

        auto resp = conn->m_udb_client->write_chunks(uh::protocol::write_chunks::request
                                                             { chunk_sizes, std::span<const char>(doc->data,
                                                               doc->size ) });

        char* returned_key = new char[64]{};
        std::memcpy(returned_key, resp.hashes.data(), 64);

        key->key = returned_key;
        key->size = 64;

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_get(UDB_CONNECTION* conn,
                   const UDB_KEY** key,
                   UDB_DOCUMENT** doc,
                   size_t n)
{
    try
    {
        for (size_t index = 0; index < n; index++)
        {
            uh::protocol::read_chunks::request req { .hashes = std::span<const char>(key[index]->key, 64)};
            auto result = conn->m_udb_client->read_chunks(req);

            /* BUG: Agency Node loses the chunk size information. */
            /* failure reading compression header should not be the error, instead couldn't read the hash should be the error */
//          auto retrieved_size = result.chunk_sizes.front();

            auto returned_data_size = std::get<std::vector<char>>(result.data).size();

            char* returned_data = new char[returned_data_size];
            std::memcpy(returned_data, std::get<std::vector<char>>(result.data).data(),
                        returned_data_size );

            doc[index] = new UDB_DOCUMENT(returned_data, returned_data_size);
        }

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_SUCCESS;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_get_one(UDB_CONNECTION* conn,
                       const UDB_KEY* key,
                       UDB_DOCUMENT* doc)
{
    try
    {
        uh::protocol::read_chunks::request req { .hashes = std::span<const char>(key->key, 64)};
        auto result = conn->m_udb_client->read_chunks(req);

        auto returned_data_size = std::get<std::vector<char>>(result.data).size();

        char* returned_data = new char[returned_data_size]{};
        std::memcpy(returned_data, std::get<std::vector<char>>(result.data).data(),
                    returned_data_size );

        doc->data = returned_data;
        doc->size = returned_data_size;

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_SUCCESS;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
