#pragma once

#include <chrono>

namespace uh::cluster::rest::utils::time
{

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
    private:
        std::chrono::system_clock::time_point m_time;
    };

} // uh::cluster::rest::utils::time