#pragma once

#include <chrono>
#include <string>

namespace uh::cluster::rest::utils::time
{
    enum class DateFormat
    {
        RFC822, //for http headers
        AutoDetect
    };

    enum class Month
    {
        January = 0,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December
    };

    enum class DayOfWeek
    {
        Sunday = 0,
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday
    };

    class date_time
    {
    public:
        date_time() = default;
        ~date_time() = default;

        [[nodiscard]] date_time now() const;
        [[nodiscard]] std::string to_gmt_string(DateFormat date) const;
        std::string to_gmt_string(const char* formatStr) const;
        [[nodiscard]] tm convert_time_stamp_to_gmt_struct() const;


    private:
        std::chrono::system_clock::time_point m_time;
    };

} // uh::cluster::rest::utils::time