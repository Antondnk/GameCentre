#pragma once

#include <string>

namespace gc {

// Время в минутах от 1970-01-01 00:00 (локальное время).
using Minutes = long long;

constexpr int kMinutesPerDay = 24 * 60;
// «Ночное» окно: для ночного тарифа ПК и ограничения для несовершеннолетних.
constexpr int kNightStartMinute = 23 * 60;
constexpr int kNightEndMinute = 6 * 60;

namespace timeutil {

long long days_from_civil(int year, unsigned month, unsigned day);
void civil_from_days(long long days, int& year, unsigned& month, unsigned& day);

Minutes now_minutes();
Minutes parse_datetime(const std::string& text);  // "ГГГГ-ММ-ДД ЧЧ:ММ"
long long parse_date_days(const std::string& text);  // "ГГГГ-ММ-ДД"
std::string format_datetime(Minutes minutes);
std::string format_date(long long days);

}  // namespace timeutil
}  // namespace gc
