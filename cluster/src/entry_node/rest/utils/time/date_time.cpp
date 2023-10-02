#include "date_time.h"
#include <time.h>

namespace uh::cluster::rest::utils::time
{

    static const char* RFC822_DATE_FORMAT_STR_MINUS_Z = "%a, %d %b %Y %H:%M:%S";

    std::string date_time::to_gmt_string(DateFormat format) const
    {
        switch (format)
        {
            case DateFormat::RFC822:
            {
                //Windows erroneously drops the local timezone in for %Z
                std::string rfc822GmtString = to_gmt_string(RFC822_DATE_FORMAT_STR_MINUS_Z);
                rfc822GmtString += " GMT";
                return rfc822GmtString;
            }
            default:
                throw std::runtime_error("unrecognized dateformat encountered");
        }
    }

    std::string date_time::to_gmt_string(const char* formatStr) const
    {
        struct tm gmtTimeStamp = convert_time_stamp_to_gmt_struct();

        char formattedString[100];
        std::strftime(formattedString, sizeof(formattedString), formatStr, &gmtTimeStamp);
        return formattedString;
    }

    tm date_time::convert_time_stamp_to_gmt_struct() const
    {
        std::time_t time = std::chrono::system_clock::to_time_t(m_time);
        struct tm gmtTimeStamp;
        gmtime_r(&time, &gmtTimeStamp);

        return gmtTimeStamp;
    }

    date_time date_time::now() const
    {
        date_time dateTime;
        dateTime.m_time = std::chrono::system_clock::now();
        return dateTime;
    }

} // uh::cluster::rest::utils::time