#include "TimeUtils.h"

#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "Exceptions.h"

namespace gc::timeutil {

// Алгоритм days_from_civil / civil_from_days (H. Hinnant), григорианский календарь.
long long days_from_civil(int year, unsigned month, unsigned day) {
    year -= month <= 2 ? 1 : 0;
    const long long era = (year >= 0 ? year : year - 399) / 400;
    const unsigned year_of_era = static_cast<unsigned>(year - era * 400);
    const unsigned day_of_year = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
    return era * 146097 + static_cast<long long>(day_of_era) - 719468;
}

void civil_from_days(long long days, int& year, unsigned& month, unsigned& day) {
    days += 719468;
    const long long era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned day_of_era = static_cast<unsigned>(days - era * 146097);
    const unsigned year_of_era =
        (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
    const unsigned day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
    const unsigned mp = (5 * day_of_year + 2) / 153;
    day = day_of_year - (153 * mp + 2) / 5 + 1;
    month = mp < 10 ? mp + 3 : mp - 9;
    year = static_cast<int>(year_of_era + era * 400) + (month <= 2 ? 1 : 0);
}

namespace {

long long checked_days(int year, int month, int day) {
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        throw InvalidInputException("Некорректная дата");
    }
    const long long days = days_from_civil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
    int y = 0;
    unsigned m = 0;
    unsigned d = 0;
    civil_from_days(days, y, m, d);
    if (y != year || static_cast<int>(m) != month || static_cast<int>(d) != day) {
        throw InvalidInputException("Несуществующая дата");
    }
    return days;
}

}  // namespace

Minutes now_minutes() {
    const std::time_t t = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    return days_from_civil(local.tm_year + 1900, static_cast<unsigned>(local.tm_mon + 1),
                           static_cast<unsigned>(local.tm_mday)) * kMinutesPerDay +
           local.tm_hour * 60 + local.tm_min;
}

Minutes parse_datetime(const std::string& text) {
    int y = 0;
    int mo = 0;
    int d = 0;
    int h = 0;
    int mi = 0;
    if (std::sscanf(text.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) != 5 || h < 0 || h > 23 ||
        mi < 0 || mi > 59) {
        throw InvalidInputException("Ожидается формат ГГГГ-ММ-ДД ЧЧ:ММ, получено: " + text);
    }
    return checked_days(y, mo, d) * kMinutesPerDay + h * 60 + mi;
}

long long parse_date_days(const std::string& text) {
    int y = 0;
    int mo = 0;
    int d = 0;
    if (std::sscanf(text.c_str(), "%d-%d-%d", &y, &mo, &d) != 3) {
        throw InvalidInputException("Ожидается формат ГГГГ-ММ-ДД, получено: " + text);
    }
    return checked_days(y, mo, d);
}

std::string format_date(long long days) {
    int y = 0;
    unsigned m = 0;
    unsigned d = 0;
    civil_from_days(days, y, m, d);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << y << '-' << std::setw(2) << m << '-' << std::setw(2) << d;
    return out.str();
}

std::string format_datetime(Minutes minutes) {
    const long long days = minutes / kMinutesPerDay;
    const int minute_of_day = static_cast<int>(minutes % kMinutesPerDay);
    std::ostringstream out;
    out << format_date(days) << ' ' << std::setfill('0') << std::setw(2) << minute_of_day / 60 << ':'
        << std::setw(2) << minute_of_day % 60;
    return out.str();
}

}  // namespace gc::timeutil
