#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace uh::cluster {

namespace http = boost::beast::http; // from <boost/beast/http.hpp>

class command_error {
public:
    enum type {
        success = 0,
        unknown,
        bucket_not_found,
        object_not_found,
        bucket_not_empty,
        fail,
        no_such_upload,
        malformed_xml,
        invalid_part,
        invalid_part_oder,
        entity_too_small,
        invalid_bucket_name,
        bad_upload_id,
        bad_part_number,
        too_many_elements,
        insufficient_storage,
        command_not_found,
        no_mp_uploads
    };

    explicit command_error(type t = unknown);

    const std::pair<std::string, std::string>& message() const;
    uint32_t code() const;
    static const std::pair<std::string, std::string>&
        get_code_message(uint32_t);
    type operator*() const;

private:
    type m_type;
};

class command_exception : public std::exception {
public:
    command_exception() = default;
    explicit command_exception(
        http::status, command_error::type = command_error::type::unknown);
    [[nodiscard]] const http::response<http::string_body>&
    get_response_specific_object();

    [[nodiscard]] const char* what() const noexcept override;

private:
    http::response<http::string_body> m_res{http::status::bad_request, 11};
    command_error m_error;
};

} // namespace uh::cluster
