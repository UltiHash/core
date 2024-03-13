#include "uri.h"
#include "command_exception.h"
#include <cctype>

namespace uh::cluster {

namespace {

// check for bucket naming rules according to
// https://docs.aws.amazon.com/AmazonS3/latest/userguide/bucketnamingrules.html
bool valid_bucket_name(const std::string& name) {

    // Bucket names must be between 3 (min) and 63 (max) characters long.
    if (name.size() < 3 || name.size() > 63) {
        return false;
    }

    // Bucket names must begin and end with a letter or number.
    if (!(std::islower(name[0]) || std::isdigit(name[0]))) {
        return false;
    }

    // Bucket names can consist only of lowercase letters, numbers, dots (.),
    // and hyphens (-).
    char last = name[0];
    for (auto i = 1u; i < name.size(); ++i) {
        if (!(std::islower(name[i]) || std::isdigit(name[i]) ||
              name[i] == '.' || name[i] == '-')) {
            return false;
        }

        // Bucket names must not contain two adjacent periods.
        if (name[i] == '.' && last == '.') {
            return false;
        }

        last = name[i];
    }

    // Bucket names must not be formatted as an IP address (for example,
    // 192.168.5.4).
    boost::system::error_code ec;
    boost::asio::ip::address::from_string(name, ec);
    if (!ec) {
        return false;
    }

    // Bucket names must not start with the prefix xn--.
    // Bucket names must not start with the prefix sthree- and the prefix
    // sthree-configurator.
    if (name.starts_with("xn--") || name.starts_with("sthree-") ||
        name.starts_with("sthree-configurator-")) {
        return false;
    }

    // Bucket names must not end with the suffix -s3alias.
    // Bucket names must not end with the suffix --ol-s3.
    if (name.ends_with("-s3alias") || name.ends_with("--ol-s3")) {
        return false;
    }

    return true;
}

} // namespace

uri::uri(const http::request_parser<http::empty_body>& req) {
    if (req.get().base().version() != 11) {
        throw std::runtime_error(
            "bad http version. support exists only for HTTP 1.1.\n");
    }

    m_method = req.get().method();

    auto target = req.get().target();

    auto query_index = target.find('?');

    if (query_index != std::string::npos) {
        // extract query string
        m_url.set_encoded_query(target.substr(query_index + 1));

        // extract path
        m_url.set_encoded_path(target.substr(0, query_index));
    } else {
        // extract path
        m_url.set_encoded_path(target.substr(0));
    }

    extract_bucket_and_object();
    extract_query_parameters();
}

const std::string& uri::get_bucket_id() const { return m_bucket_id; }

const std::string& uri::get_object_key() const { return m_object_key; }

method uri::get_method() const { return m_method; }

bool uri::query_string_exists(const std::string& key) const {
    auto itr = m_query_parameters.find(key);
    if (itr != m_query_parameters.end()) {
        return true;
    } else {
        return false;
    }
}

const std::string& uri::get_query_string_value(const std::string& key) const {
    return m_query_parameters.at(key);
}

const std::map<std::string, std::string>& uri::get_query_parameters() const {
    return m_query_parameters;
}

void uri::extract_query_parameters() {
    for (const auto& param : m_url.params()) {
        m_query_parameters[param.key] = param.value;
    }
}

void uri::extract_bucket_and_object() {
    for (const auto& seg : m_url.segments()) {
        if (m_bucket_id.empty())
            m_bucket_id = seg;
        else
            m_object_key += seg + '/';
    }

    if (!m_object_key.empty())
        m_object_key.pop_back();

    // check bucket id and object key for validity
    if (!m_bucket_id.empty()) {
        if (m_bucket_id.size() < 3 || m_bucket_id.size() > 63) {
            throw command_exception(http::status::bad_request,
                                    command_error::invalid_bucket_name);
        }

        if (!valid_bucket_name(m_bucket_id)) {
            throw command_exception(http::status::bad_request,
                                    command_error::invalid_bucket_name);
        }
    }
}

} // namespace uh::cluster
