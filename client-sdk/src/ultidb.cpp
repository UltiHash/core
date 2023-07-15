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

struct UDB_OBJECT_STRUCT
{
    char* key;
    size_t key_size;
    char* value;
    size_t value_size;
    char** labels;
    size_t label_count;

    UDB_OBJECT_STRUCT() : key(nullptr), key_size(0), value(nullptr), value_size(0),
                            labels(nullptr), label_count(0)
    {};

    UDB_OBJECT_STRUCT(char* rec_key, size_t rec_key_size, char* rec_value, size_t rec_value_size,
                        char** rec_labels, size_t rec_label_count) :
        key(rec_key),
        key_size(rec_key_size),
        value(rec_value),
        value_size(rec_value_size),
	    labels(rec_labels),
        label_count(rec_label_count)
    {}
};

// ---------------------------------------------------------------------

struct UDB_OBJECTS
{
    std::vector<UDB_OBJECT_STRUCT*> objects {};

    UDB_OBJECTS() = default;

    ~UDB_OBJECTS() = default;
};

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
    std::vector<UDB_DATA> start_key;
    UDB_DATA end_key;
    char** labels;
    size_t label_count;
    size_t key_count;
    UDB_READ_QUERY_TYPE query_type;


    UDB_READ_QUERY_STRUCT() : start_key(),
    end_key(), labels(nullptr), label_count(0), key_count(0),
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
        case 4: return "A key was already set previously.";
        case 5: return "A key wasn't set on read query structure";
        case 6: return "Error connecting to the server.";
        default: return "Unknown Error";
    }
}

// ---------------------------------------------------------------------

struct UDB_CONFIG_STRUCT
{
    UDB_CONFIG_STRUCT() :
            hostname(nullptr),
            port(0)
    {};

    const char* hostname;
    uint16_t port;

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

UDB_RESULT udb_config_set_host_node(UDB_CONFIG* cfg, const char *hostname, uint16_t port)
{
    cfg->hostname = hostname;
    cfg->port = port;
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
        case UDB_RESULT::UDB_SERVER_CONNECTION_ERROR:
            return "error connecting to the server";
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
        try
        {
            m_udb_client = udb->m_client_factory->create();
        }
        catch(const std::exception& e)
        {
            THROW(uh::util::exception, Exception_Messsage(UDB_SERVER_CONNECTION_ERROR));
        }
    }

    ~UDB_CONNECTION_STRUCT() = default;
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
        if (e.what() == std::string(Exception_Messsage(UDB_SERVER_CONNECTION_ERROR)))
            error = UDB_SERVER_CONNECTION_ERROR;
        else
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
        if (!conn->m_udb_client->valid())
            THROW(uh::util::exception, "cannot reach server");
        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_SERVER_CONNECTION_ERROR;
        return UDB_RESULT::UDB_SERVER_CONNECTION_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_OBJECT* udb_init_object(char* key, size_t key_size, char* value, size_t value_size,
                                char** labels, size_t label_count)
{
    try
    {
        return new UDB_OBJECT_STRUCT(key, key_size, value, value_size, labels, label_count);
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_object(UDB_OBJECT_STRUCT* ptr_to_document_ptr)
{
    try
    {
        delete ptr_to_document_ptr;
        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT_ERROR;
    }
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

void udb_write_query_add_object(UDB_WRITE_QUERY* write_query, UDB_OBJECT* object)
{
    write_query->objects.push_back(object);
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY* ptr_to_write_query_ptr)
{
    try
    {
        delete ptr_to_write_query_ptr;
        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT::UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

struct UDB_WRITE_QUERY_RESULTS
{
    std::span<uint32_t> effective_sizes;
    std::span<uint8_t> return_code;

    ~UDB_WRITE_QUERY_RESULTS()
    {
        delete [] effective_sizes.data();
        delete [] return_code.data();
    }
};

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_write_query_results(UDB_WRITE_QUERY_RESULTS* results)
{
    try
    {
        delete results;
        return UDB_RESULT::UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT::UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

size_t udb_get_effective_sizes_count(UDB_WRITE_QUERY_RESULTS* results)
{
    return results->effective_sizes.size();
}

// ---------------------------------------------------------------------

uint32_t udb_get_effective_size(UDB_WRITE_QUERY_RESULTS* results, size_t index)
{
    return results->effective_sizes[index];
}

// ---------------------------------------------------------------------

size_t udb_get_return_code_count(UDB_WRITE_QUERY_RESULTS* results)
{
    return results->return_code.size();
}

// ---------------------------------------------------------------------

uint8_t udb_get_return_code(UDB_WRITE_QUERY_RESULTS* results, size_t index)
{
    return results->return_code[index];
}

// ---------------------------------------------------------------------

UDB_WRITE_QUERY_RESULTS* udb_put(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query)
{
    try
    {
        // TODO: fill it in a buffer and then send it after the buffer is filled instead of sending
        //  all objects at once
        std::vector<uint16_t> key_sizes;
        std::vector<uint32_t> value_sizes;
        std::vector<uint8_t> label_count;
        std::vector<uint8_t> label_sizes;

        // TODO: reserve the memory and then memcpy into this reserved memory since we know the size of write_query
        std::vector<char> data;

        for (const auto& object : write_query->objects)
        {
            key_sizes.push_back(object->key_size);
            data.insert(data.end(), object->key, object->key + object->key_size);

            data.push_back(uh::util::INSERT_UPDATE);

            value_sizes.push_back(object->value_size);
            data.insert(data.end(), object->value, object->value + object->value_size);

            label_count.push_back(object->label_count);
            for (size_t index = 0; index < object->label_count; index++ )
            {
                auto label_size = strlen(object->labels[index]);
                label_sizes.push_back(label_size);
                data.insert(data.end(), object->labels[index], object->labels[index] + label_size);
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

        auto* results = new UDB_WRITE_QUERY_RESULTS();
        results->return_code = std::span<uint8_t>(resp.return_codes);
        results->effective_sizes = std::span<uint32_t>(resp.effective_sizes);
        resp.effective_sizes.data.release();
        resp.return_codes.data.release();

        return results;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return nullptr;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
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

UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, char* key, size_t key_size)
{
    try
    {
        if (read_query->query_type != RANGE_KEYS)
        {
            read_query->start_key.emplace_back(key, key_size);
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
        if (e.what() == std::string(Exception_Messsage(UDB_KEY_ALREADY_SET)))
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

UDB_RESULT udb_read_query_set_key_range(UDB_READ_QUERY* read_query, char* start_key, size_t start_size,
                                        char* end_key, size_t end_key_count)
{
    try
    {
        if (read_query->query_type == RANGE_KEYS || read_query->query_type == NOT_DEFINED)
        {
            read_query->start_key.emplace_back(start_key, start_size);
            read_query->end_key = UDB_DATA(end_key, end_key_count);
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
        if (e.what() == std::string(Exception_Messsage(UDB_KEY_ALREADY_SET)))
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

UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY* read_query_ptr_container)
{
    try
    {
        delete read_query_ptr_container;

        return UDB_RESULT_SUCCESS;
    }
    catch(const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

struct UDB_READ_QUERY_RESULTS
{
    std::vector<UDB_READ_QUERY_RESULT> results_container;
    size_t counter;

    UDB_READ_QUERY_RESULTS() : results_container(), counter(0) {}

    ~UDB_READ_QUERY_RESULTS() = default;
};

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_read_query_results(UDB_READ_QUERY_RESULTS* results)
{
    try
    {
        delete [] results->results_container[0].key;
        delete results;

        return UDB_RESULT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return UDB_RESULT::UDB_RESULT_ERROR;
    }
}

// ---------------------------------------------------------------------

UDB_READ_QUERY_RESULTS* udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query)
{
    try
    {
        std::vector<uint16_t> start_key_sizes {};
        std::vector<uint16_t> end_key_sizes {};
        std::vector<uint16_t> single_key_sizes {};
        std::vector<uint8_t> label_counts {};
        std::vector<uint8_t> label_sizes {};
        std::vector<char> data {};

        switch (read_query->query_type) {
            case SINGLE_KEY:

                start_key_sizes.push_back(0u);
                end_key_sizes.push_back(0u);

                single_key_sizes.push_back(read_query->start_key.front().size);
                data.insert(data.end(), read_query->start_key.front().data,
                            read_query->start_key.front().data + read_query->start_key.front().size);

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
                    start_key_sizes.push_back(0u);
                    end_key_sizes.push_back(0u);
                    single_key_sizes.push_back(read_query->start_key[index].size);
                    data.insert(data.end(), read_query->start_key[index].data,
                                read_query->start_key[index].data + read_query->start_key[index].size);

                    label_counts.push_back(read_query->label_count);

                    for (auto count = 0; count < read_query->label_count; count++)
                    {
                        auto label_size = sizeof(read_query->labels[count]);
                        label_sizes.push_back(label_size);
                        data.insert(data.end(), read_query->labels[count], read_query->labels[count] + label_size);
                    }
                }

                break;

            case RANGE_KEYS:

                // not implemented yet
                throw std::runtime_error("not implemented");

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
        auto* read_query_results = new UDB_READ_QUERY_RESULTS();

        for (auto rr = read_response.next(); rr != nullptr; rr = read_response.next())
        {
            char* key = rr->key.data();
            size_t key_size = rr->key.size();

            char* value = rr->value.data();
            size_t value_size = rr->value.size();

            char** labels = nullptr;
            if (rr->labels.size > 0)
                labels = new char*[rr->labels.size];

            for (size_t index = 0; index < rr->labels.size; index++)
            {
                const auto str = rr->labels.data [index];
                if (!str.ends_with('\0'))
                {
                    auto* label = new char[rr->labels.data[index].size() + 1];
                    std::memcpy(label, str.data(), rr->labels.data[index].size());
                    label[rr->labels.data[index].size()] = '\0';

                    labels[index] = label;
                }
                else
                {
                    auto* label = new char[rr->labels.data[index].size()];
                    std::memcpy(label, str.data(), rr->labels.data[index].size());
                    labels[index] = label;
                }
            }
            auto labels_count = rr->labels.size;

            read_query_results->results_container.emplace_back(key, key_size, value, value_size, labels, labels_count);
        }

        // Free the ospan since object will free the data for us
        std::get<0>(resp.data).data.release();

        return read_query_results;
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return nullptr;
    }
    catch (const std::exception& e)
    {
        if (e.what() == std::string(Exception_Messsage(UDB_UNINITIALIZED_KEY)))
        {
            error = UDB_UNINITIALIZED_KEY;
        }
        else
        {
            error = UDB_RESULT_ERROR;
        }

        return nullptr;
    }
}

// ---------------------------------------------------------------------

bool udb_results_next(UDB_READ_QUERY_RESULTS* results_container, UDB_READ_QUERY_RESULT** result_ptr)
{

    if (results_container->counter > results_container->results_container.size()-1)
    {
        results_container->counter = 0;
        return false;
    }

    *result_ptr = &results_container->results_container[results_container->counter];

    results_container->counter++;

    return true;
}

// ---------------------------------------------------------------------

size_t udb_get_results_count(UDB_READ_QUERY_RESULTS* results)
{
    return results->results_container.size();
}


// ---------------------------------------------------------------------

UDB_READ_QUERY_RESULT* udb_get_result(UDB_READ_QUERY_RESULTS* results, size_t index)
{
    return &results->results_container[index];
}

// ---------------------------------------------------------------------

UDB_DATA* udb_get_key(UDB_OBJECT* obj)
{
    try
    {
        return new UDB_DATA(obj->key, obj->key_size);
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return nullptr;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

UDB_DATA* udb_get_value(UDB_OBJECT* obj)
{
    try
    {
        return new UDB_DATA(obj->value, obj->value_size);
    }
    catch (const std::bad_alloc& e)
    {
        error = UDB_BAD_ALLOCATION;
        return nullptr;
    }
    catch (const std::exception& e)
    {
        error = UDB_RESULT_ERROR;
        return nullptr;
    }
}

// ---------------------------------------------------------------------

size_t udb_get_labels_count(UDB_OBJECT* obj)
{
    return obj->label_count;
}

// ---------------------------------------------------------------------

char* udb_get_label(UDB_OBJECT* obj, size_t label_index)
{
    return obj->labels[label_index];
}

// ---------------------------------------------------------------------

UDB_RESULT udb_destroy_udb_data(UDB_DATA* data)
{
    delete data;
    return UDB_RESULT_SUCCESS;
}

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif
