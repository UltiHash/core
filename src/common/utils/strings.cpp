#include "strings.h"

#include <boost/beast/core/detail/base64.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/url.hpp>
#include <boost/url/encode.hpp>
#include <sstream>

using namespace boost;

namespace uh::cluster {

std::vector<std::string_view> split(std::string_view data, char delimiter) {
    auto split =
        data | std::ranges::views::split(delimiter) |
        std::ranges::views::transform([](auto&& str) {
            return std::string_view(&*str.begin(), std::ranges::distance(str));
        });

    return {split.begin(), split.end()};
}

std::vector<char> base64_decode(std::string_view b64) {
    std::vector<char> rv(beast::detail::base64::decoded_size(b64.size()));

    auto sizes = beast::detail::base64::decode(&rv[0], b64.data(), b64.size());
    rv.resize(sizes.first);

    return rv;
}

constexpr boost::urls::grammar::lut_chars custom_unreserved_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-._~/";

std::string url_encode(const std::string& str_to_encode) noexcept {
    auto encoded_string =
        boost::urls::encode(str_to_encode, custom_unreserved_chars);

    return encoded_string;
}

bool to_bool(std::string str_to_eval) {
    std::transform(str_to_eval.begin(), str_to_eval.end(), str_to_eval.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    std::istringstream is(str_to_eval);
    bool b;
    is >> std::boolalpha >> b;
    return b;
}

std::string xml_escape(const std::string& str_to_encode) {
    std::ostringstream oss;
    boost::property_tree::ptree pt;
    pt.put("root", str_to_encode);

    boost::property_tree::write_xml(oss, pt, boost::property_tree::xml_writer_make_settings<std::string>(' ', 4));
    std::string result = oss.str();

    // removing these tags seems cumbersome, but it's better than
    // manually having to maintain an XML escaper where boost internally
    // already has one
    auto start = result.find("<root>") + 6;
    auto end = result.rfind("</root>");
    return result.substr(start, end - start);
}


} // namespace uh::cluster
