/* Implementation of UltiDB APIs. */

#include "../include/ultidb.h"
#include <thread>
#include <protocol/client_pool.h>
#include <protocol/client_factory.h>
#include <net/plain_socket.h>
#include <util/ospan.h>
#include <util/structured_queries.h>
#include <iostream>
#include <vector>

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
    OWNING_TYPE underlying_pointers;

    UDB_DOCUMENT_STRUCT() : key(nullptr), value(nullptr), labels(nullptr), label_count(0),
    underlying_pointers(non_owning) {};

    explicit UDB_DOCUMENT_STRUCT(OWNING_TYPE owning_t) :  key(nullptr), value(nullptr), labels(nullptr), label_count(0),
    underlying_pointers(owning_t)
    {}

    UDB_DOCUMENT_STRUCT(UDB_DATA* rec_key, UDB_DATA* rec_value, char** rec_labels, size_t rec_label_count) :
        key(rec_key),
        value(rec_value),
	labels(rec_labels),
        label_count(rec_label_count),
        underlying_pointers(non_owning)
    {}

    ~UDB_DOCUMENT_STRUCT()
    {
        if (underlying_pointers == owning)
        {
            delete [] key->data;
            key->data = nullptr;
            delete key;
            key = nullptr;
            delete [] value->data;
            value->data = nullptr;
            delete value;
            for (size_t index = 0; index < label_count; index++)
            {
                delete [] labels[index];
                labels[index] = nullptr;
            }
            delete [] labels;
            labels = nullptr;
        }
    }
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

UDB_DOCUMENTS* udb_create_documents_container()
{
    return new UDB_DOCUMENTS();
}

UDB_RESULT udb_add_document(UDB_DOCUMENTS* documents_container, UDB_DOCUMENT* document)
{
    try
    {
        documents_container->documents.push_back(document);
        return UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

UDB_RESULT udb_destroy_documents_container(UDB_DOCUMENTS** documents_container)
{
    try
    {
        delete *documents_container;
        *documents_container = nullptr;

        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

typedef enum : uint8_t
{
    SINGLE_KEY = 0,

    MULTIPLE_KEYS,

    RANGE_KEYS,

    NOT_DEFINED

} UDB_READ_QUERY_TYPE;

// ---------------------------------------------------------------------

struct UDB_READ_QUERY_STRUCT
{
    std::vector<UDB_DATA*> start_key {};
    UDB_DATA* end_key;
    char** labels;
    size_t label_count;
    size_t key_count;
    UDB_READ_QUERY_TYPE query_type;

    UDB_READ_QUERY_STRUCT() :
    end_key(nullptr), labels(nullptr), label_count(0), key_count(0),
    query_type(UDB_READ_QUERY_TYPE::NOT_DEFINED)
    {}
};

// ---------------------------------------------------------------------

const char* get_error_message()
{
    switch(error)
    {
        case 0 : return "Successful Operation.";
        case 2: return "Buffer Overflow";
        case 3: return "Bad memory allocation";
        default: return "Unknown Error";
    }
}

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

const char* get_sdk_version()
{
    return SDK_VERSION;
}

// ---------------------------------------------------------------------

const char* get_sdk_name()
{
    return SDK_NAME;
}

// ---------------------------------------------------------------------

constexpr const char* Exception_Messsage(uint8_t n)
{
    switch (n)
    {
        case UDB_RESULT::UDB_RESULT_ERROR:
            return "Cannot get uninitialized read query.";
        case UDB_RESULT::UDB_KEY_ALREADY_SET:
            return "Key type was already previously set..";
        default:
            return "The given enum doesn't have any string associated with it";
    }
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
        return UDB_RESULT_SUCCESS;
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

void udb_write_query_add_document(UDB_WRITE_QUERY* write_query, UDB_DOCUMENT* document)
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
        // TODO: fill it in a buffer and then send it after the buffer is filled instead of sending all docuents at once
        std::vector<uint16_t> key_sizes;
        std::vector<uint32_t> value_sizes;
        std::vector<uint8_t> label_count;
        std::vector<uint8_t> label_sizes;

        // TODO: reserve the memory and then memcpy into this reserved memory since we know the size of write_query
        std::vector<char> data;

        for (const auto& document : write_query->documents)
        {
            key_sizes.push_back(document->key->size);
            data.insert(data.end(), document->key->data, document->key->data + document->key->size);

            value_sizes.push_back(document->value->size);
            data.insert(data.end(), document->value->data, document->value->data + document->value->size);

            label_count.push_back(document->label_count);
            for (size_t index = 0; index < document->label_count; index++ )
            {
                auto label_size = strlen(document->labels[index]);
                label_sizes.push_back(label_size);
                data.insert(data.end(), document->labels[index], document->labels[index] + label_size);
            }
        }

        auto resp = conn->m_udb_client->write_kv(uh::protocol::write_key_value::request
                                                         {
                                                                 std::span<uint16_t>(key_sizes.data(), key_sizes.size()),
                                                                 std::span<uint32_t>(value_sizes.data(), value_sizes.size()),
                                                                 std::span<uint8_t>(label_count.data(), label_count.size()),
                                                                 std::span<uint8_t>(label_sizes.data(), label_sizes.size()),
                                                                 std::span<char>(data.data(), data.size())
                                                         });

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_READ_QUERY* udb_create_read_query()
{
    try
    {
        return new UDB_READ_QUERY();
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT::UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, UDB_DATA* key)
{
    try
    {
        if (read_query->query_type != RANGE_KEYS)
        {
            read_query->start_key.push_back(key);
            read_query->key_count++;

            switch (read_query->query_type)
            {
                case NOT_DEFINED:
                    read_query->query_type = SINGLE_KEY;
                    break;
                case SINGLE_KEY:
                    read_query->query_type = MULTIPLE_KEYS;
                    break;
            }
        }
        else
        {
            throw std::runtime_error(Exception_Messsage(UDB_KEY_ALREADY_SET));
        }

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        if (e.what() == Exception_Messsage(UDB_KEY_ALREADY_SET))
        {
            error = UDB_KEY_ALREADY_SET;
        }
        else
        {
            error = UDB_RESULT_ERROR;
        }

        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_read_query_set_key_range(UDB_READ_QUERY* read_query, UDB_DATA* start_key, UDB_DATA* end_key)
{
    try
    {
        if (read_query->query_type == RANGE_KEYS || read_query->query_type == NOT_DEFINED)
        {
            read_query->start_key.push_back(start_key);
            read_query->end_key = end_key;
            read_query->query_type = RANGE_KEYS;
        }
        else
        {
            throw std::runtime_error(Exception_Messsage(UDB_KEY_ALREADY_SET));
        }

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        if (e.what() == Exception_Messsage(UDB_KEY_ALREADY_SET))
        {
            error = UDB_KEY_ALREADY_SET;
        }
        else
        {
            error = UDB_RESULT_ERROR;
        }

        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_read_query_add_label(UDB_READ_QUERY* read_query, char** labels, size_t label_count)
{
    try
    {
        read_query->labels = labels;
        read_query->label_count = label_count;

        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY** read_query_ptr_container)
{
    try
    {
        delete *read_query_ptr_container;
        *read_query_ptr_container = nullptr;

        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query, UDB_DOCUMENTS* udb_documents)
{
    try
    {

        std::vector<uint16_t> start_key_sizes {};
        std::vector<uint16_t> end_key_sizes {};
        std::vector<uint16_t> single_key_sizes {};
        std::vector<uint8_t> label_counts {};
        std::vector<uint8_t> label_sizes {};
        std::vector<char> data {};

        switch (static_cast<uint8_t>(read_query->query_type)) {
            case SINGLE_KEY:

                start_key_sizes.push_back(0u);
                end_key_sizes.push_back(0u);

                single_key_sizes.push_back(read_query->start_key.front()->size);
                data.insert(data.end(), read_query->start_key.front()->data,
                            read_query->start_key.front()->data + read_query->start_key.front()->size);

                label_counts.push_back(read_query->label_count);

                for (size_t index = 0; index < read_query->label_count; index++)
                {
                    auto label_size = sizeof(read_query->labels[index]);
                    label_sizes.push_back(label_size);
                    data.insert(data.end(), read_query->labels[index], read_query->labels[index] + label_size);
                }

                break;
            case MULTIPLE_KEYS:


                for (size_t index = 0; index < read_query->start_key.size(); index++)
                {
                    single_key_sizes.push_back(read_query->start_key[index]->size);
                    data.insert(data.end(), read_query->start_key[index]->data,
                                read_query->start_key[index]->data + read_query->start_key[index]->size);

                    label_counts.push_back(read_query->label_count);

                    for (int count = 0; count < read_query->label_count; count++)
                    {
                        auto label_size = sizeof(read_query->labels[count]);
                        label_sizes.push_back(label_size);
                        data.insert(data.end(), read_query->labels[count], read_query->labels[count] + label_size);
                    }
                    start_key_sizes.push_back(0);
                    end_key_sizes.push_back(0);

                }

                break;
            case RANGE_KEYS:

                start_key_sizes.push_back(read_query->start_key.front()->size);
                data.insert(data.end(), read_query->start_key.front()->data,
                            read_query->start_key.front()->data + read_query->start_key.front()->size);


                end_key_sizes.push_back(read_query->end_key->size);
                data.insert(data.end(), read_query->end_key->data,
                            read_query->end_key->data + read_query->end_key->size);


                label_counts.push_back(read_query->label_count);

                for (auto index = 0; index < read_query->label_count; index++)
                {
                    auto label_size = sizeof(read_query->labels[index]);
                    label_sizes.push_back(label_size);
                    data.insert(data.end(), read_query->labels[index], read_query->labels[index] + label_size);
                }
                single_key_sizes.push_back(0);

                break;

            case NOT_DEFINED:
                throw std::logic_error(Exception_Messsage(UDB_UNINITIALIZED_KEY));
            default:
                throw std::runtime_error("Unrecognized error.");
        }

        auto resp = conn->m_udb_client->read_kv(uh::protocol::read_key_value::request
            {
                std::span<uint16_t>(start_key_sizes.data(), start_key_sizes.size()),
                std::span<uint16_t>(end_key_sizes.data(), end_key_sizes.size()),
                std::span<uint16_t>(single_key_sizes.data(), single_key_sizes.size()),
                std::span<uint8_t>(label_counts.data(), label_counts.size()),
                std::span<uint8_t>(label_sizes.data(), label_sizes.size()),
                std::span<char>(data.data(), data.size())
            });

        uh::util::structured_queries <uh::protocol::read_key_value::response> read_response(resp);

        // TODO; do not copy data if possible
        for (auto rr = read_response.next(); rr != nullptr; rr = read_response.next())
        {

            auto* new_document = new UDB_DOCUMENT(owning);
            UDB_DATA* key_data;
            if (read_query->query_type == SINGLE_KEY)
            {
                auto* key_value = new char[read_query->start_key[0]->size + 1];
                std::memcpy(key_value, read_query->start_key[0]->data, read_query->start_key[0]->size);
                key_value[read_query->start_key[0]->size] = '\0';
                key_data = new UDB_DATA(key_value, read_query->start_key[0]->size);;
            }
            else
            {
                key_data = new UDB_DATA(rr->key.data(), rr->key.size());
            }

            auto new_data = new UDB_DATA(rr->value.data(), rr->value.size());

            new_document->key = key_data;
            new_document->value = new_data;

            char** labels_pointer = nullptr;
            if (rr->labels.size > 0)
                labels_pointer = new char*[rr->labels.size];
            for (size_t index = 0; index < rr->labels.size; index++)
            {
                const auto str = rr->labels.data [index];
                if (!str.ends_with('\0'))
                {
                    auto* label = new char[rr->labels.data[index].size() + 1];
                    std::memcpy(label, str.data(), rr->labels.data[index].size());
                    label[rr->labels.data[index].size()] = '\0';
                    labels_pointer[index] = label;
                }
                else
                {
                    auto* label = new char[rr->labels.data[index].size()];
                    std::memcpy(label, str.data(), rr->labels.data[index].size());
                    labels_pointer[index] = label;
                }
            }
            new_document->labels = labels_pointer;
            new_document->label_count = rr->labels.size;
            udb_documents->documents.push_back(new_document);
            udb_documents->count++;
        }

        // Free the ospan so that we don't delete twice, also do not copy the key and labels
        std::get<0>(resp.data).data.release();

        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return UDB_RESULT::UDB_BAD_ALLOCATION;
    }
    catch (const std::exception& e)
    {
        if (e.what() == Exception_Messsage(UDB_UNINITIALIZED_KEY))
        {
            error = UDB_UNINITIALIZED_KEY;
        }
        else
        {
            error = UDB_RESULT_ERROR;
        }

        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

size_t udb_get_documents_count(UDB_DOCUMENTS* docs)
{
    return docs->count;
}

// ---------------------------------------------------------------------

UDB_DOCUMENT* udb_get_document(UDB_DOCUMENTS* docs, size_t index)
{
    return docs->documents[index];
}

// ---------------------------------------------------------------------

UDB_DATA* udb_get_key(UDB_DOCUMENT* doc)
{
    return doc->key;
}

// ---------------------------------------------------------------------

UDB_DATA* udb_get_value(UDB_DOCUMENT* doc)
{
    std::string test_str;
    for (size_t i=0; i< doc->value->size; i++)
    {
        test_str.push_back(doc->value->data[i]);
    }
    return doc->value;
}

// ---------------------------------------------------------------------

size_t udb_get_labels_count(UDB_DOCUMENT* doc)
{
    return doc->label_count;
}

// ---------------------------------------------------------------------

char* udb_get_label(UDB_DOCUMENT* doc, size_t label_index)
{
    return doc->labels[label_index];
}

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
