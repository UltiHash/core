#pragma once

#include <chrono>
#include <string>
#include <ctime>
#include <stdexcept>

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

//    std::string get_formatted_date() const
//    {
//        std::time_t t = std::time(nullptr);
//        std::tm tm = *std::localtime(&t);
//
//        char buffer[9];
//        std::strftime(buffer, sizeof(buffer), "%Y%m%d", &tm);
//
//        return {buffer, sizeof(buffer) - 1};
//    }

    class date_time
    {
    public:
        date_time() = default;
        ~date_time() = default;

        [[nodiscard]] static date_time now();
        [[nodiscard]] std::string to_gmt_string(DateFormat date) const;
        [[nodiscard]] std::string to_gmt_string(const char* formatStr) const;
        [[nodiscard]] std::string get_date() const;

    private:
        [[nodiscard]] tm convert_time_stamp_to_gmt_struct() const;
        std::chrono::system_clock::time_point m_time;
    };

} // uh::cluster::rest::utils::time