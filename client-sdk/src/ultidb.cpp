/* Implementation of UltiDB APIs. */

#include "../include/ultidb.h"
#include <functional>
#include <thread>
#include <utility>
#include <protocol/client_pool.h>
#include <protocol/client_factory.h>
#include <net/plain_socket.h>
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

struct UDB_DOCUMENT_STRUCT
{
    UDB_DATA* key;
    UDB_DATA* value;
    char** labels;
    size_t label_count;

    UDB_DOCUMENT_STRUCT() : key(nullptr), value(nullptr), labels(nullptr), label_count(0)
    {}

    UDB_DOCUMENT_STRUCT(UDB_DATA* rec_key, UDB_DATA* rec_value, char** rec_labels, size_t rec_label_count) :
        key(rec_key),
        value(rec_value),
        labels(rec_labels),
        label_count(rec_label_count)
    {}
};

// ---------------------------------------------------------------------

struct UDB_DOCUMENTS
{
    std::vector<UDB_DOCUMENT_STRUCT*> documents {};
    size_t count;

    UDB_DOCUMENTS() : count(0)
    {}

    ~UDB_DOCUMENTS()
    {
        for (auto& item : documents)
        {
            delete item;
        }
        documents.clear();
    }
};

// ---------------------------------------------------------------------

struct UDB_READ_QUERY_STRUCT
{
    UDB_DATA** start_key;
    UDB_DATA* end_key;
    char **labels;
    int label_count;
    int key_count;
    UDB_READ_QUERY_TYPE query_type;
};

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
    cfg->hostname = hostname;
    cfg->port = port;
    cfg->connection_pool = connection_pool;
    return UDB_RESULT_SUCCESS;
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

UDB_DOCUMENT* udb_init_document(UDB_DATA* key, UDB_DATA* value, char** labels, size_t label_count)
{
    try
    {
        return new UDB_DOCUMENT_STRUCT(key, value, labels, label_count);
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
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

UDB_RESULT udb_destroy_document(UDB_DOCUMENT_STRUCT** ptr_to_document_ptr)
{
    try
    {
        delete *ptr_to_document_ptr;
        *ptr_to_document_ptr = nullptr;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

void udb_document_set_key(UDB_DOCUMENT* doc, UDB_DATA* key)
{
    doc->key = key;
}

// ---------------------------------------------------------------------

void udb_document_set_value(UDB_DOCUMENT* doc, UDB_DATA* value)
{
    doc->value = value;
}

// ---------------------------------------------------------------------

void udb_document_set_labels(UDB_DOCUMENT* doc, char** labels)
{
    doc->labels = labels;
}

// ---------------------------------------------------------------------

UDB_WRITE_QUERY* udb_create_write_query()
{
    try
    {
        return new UDB_WRITE_QUERY();
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT::UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_write_query_add_document(UDB_WRITE_QUERY* write_query, UDB_DOCUMENT* document)
{
    write_query->documents.push_back(document);
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY** ptr_to_write_query_ptr)
{
    try
    {
        delete *ptr_to_write_query_ptr;
        *ptr_to_write_query_ptr = nullptr;
        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT::UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_add(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query)
{
    try
    {
        for (const auto& document : write_query->documents)
        {
            std::vector<uint32_t> chunk_sizes; // TODO: inefficient has to change
            chunk_sizes.push_back(static_cast<uint32_t>(document->value->size));

            auto resp = conn->m_udb_client->write_chunks(uh::protocol::write_chunks::request
                { chunk_sizes, std::span<const char>(document->value->data, document->value->size ) });

            char* returned_key = new char[64]{};
            std::memcpy(returned_key, resp.hashes.data(), 64); // TODO: inefficient copy
        }
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;;
    }
}

// ---------------------------------------------------------------------

UDB_READ_QUERY* udb_create_read_query()
{}

// ---------------------------------------------------------------------

UDB_RESULT udb_read_query_add_key()
{}

// ---------------------------------------------------------------------

UDB_RESULT udb_read_query_set_key_range()
{}

// ---------------------------------------------------------------------

UDB_RESULT udb_read_query_add_label()
{}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY** read_query_ptr_container)
{}

// ---------------------------------------------------------------------



// ---------------------------------------------------------------------



// ---------------------------------------------------------------------



// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
