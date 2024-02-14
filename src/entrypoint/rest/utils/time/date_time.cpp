#include "date_time.h"

namespace uh::cluster::rest::utils::time {

static const char* RFC822_DATE_FORMAT_STR_MINUS_Z = "%a, %d %b %Y %H:%M:%S";

std::string date_time::to_gmt_string(DateFormat format) const {
    switch (format) {
    case DateFormat::RFC822: {
        // Windows erroneously drops the local timezone in for %Z
        std::string rfc822GmtString =
            to_gmt_string(RFC822_DATE_FORMAT_STR_MINUS_Z);
        rfc822GmtString += " GMT";
        return rfc822GmtString;
    }
    default:
        throw std::runtime_error("unrecognized dateformat encountered");
    }
}

std::string date_time::to_gmt_string(const char* formatStr) const {
    struct tm gmtTimeStamp = convert_time_stamp_to_gmt_struct();

    char formattedString[100];
    std::strftime(formattedString, sizeof(formattedString), formatStr,
                  &gmtTimeStamp);
    return formattedString;
}

tm date_time::convert_time_stamp_to_gmt_struct() const {
    std::time_t time = std::chrono::system_clock::to_time_t(m_time);
    struct tm gmtTimeStamp {};
    gmtime_r(&time, &gmtTimeStamp);

    return gmtTimeStamp;
}

std::string date_time::get_date() const {
    std::time_t t = std::chrono::system_clock::to_time_t(m_time);
    std::tm tm = *std::localtime(&t);

    char buffer[9];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d", &tm);

    return {buffer, sizeof(buffer) - 1};
}

date_time date_time::now() {
    date_time dateTime;
    dateTime.m_time = std::chrono::system_clock::now();
    return dateTime;
}

} // namespace uh::cluster::rest::utils::time