#pragma once

#include <array>
#include <string>
#include <vector>

#include "GameCenter.h"

namespace gc {

struct NamedValue {
    std::string name;
    double value = 0.0;
};

struct DailyValue {
    long long day = 0;  // дни от 1970-01-01
    double value = 0.0;
};

// Сводные показатели для страницы «Аналитика» и текстового отчёта.
struct AnalyticsReport {
    Money revenue_total;  // списания - возвраты
    Money deposits_total;
    Money refunds_total;
    double average_check = 0.0;
    int spend_operations = 0;
    int clients_count = 0;
    int bookings_count = 0;
    int orders_count = 0;

    std::vector<DailyValue> revenue_by_day;           // последние N суток, по возрастанию
    std::vector<NamedValue> revenue_by_category;      // по убыванию
    std::vector<NamedValue> booked_hours_by_resource;  // топ ресурсов по часам броней
    std::array<int, 24> bookings_by_hour{};           // сколько броней начинается в каждый час
    std::vector<NamedValue> top_clients;              // по сумме трат
    std::vector<NamedValue> top_products;             // по количеству проданных единиц
};

// Русское название категории транзакции (session -> "Сеансы" и т.д.).
std::string category_title(const std::string& category);

AnalyticsReport compute_analytics(const GameCenter& center, Minutes now, int days = 7);

}  // namespace gc
