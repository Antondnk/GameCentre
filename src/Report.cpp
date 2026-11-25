#include "Report.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "Analytics.h"

namespace gc {

namespace {

void section(std::ostringstream& out, const std::string& title) {
    out << "\n=== " << title << " ===\n";
}

void write_named_values(std::ostringstream& out, const std::vector<NamedValue>& values, const std::string& unit) {
    if (values.empty()) {
        out << "  (нет данных)\n";
        return;
    }
    for (const NamedValue& item : values) {
        out << "  " << std::left << std::setw(32) << item.name << std::right << std::fixed << std::setprecision(2)
            << std::setw(10) << item.value << " " << unit << "\n";
    }
}

}  // namespace

std::string build_text_report(const GameCenter& center, Minutes now) {
    const AnalyticsReport analytics = compute_analytics(center, now, 7);
    std::ostringstream out;
    out << "ОТЧЁТ ИГРОВОГО ЦЕНТРА\nСформирован: " << timeutil::format_datetime(now) << "\n";

    section(out, "Сводка");
    out << "  Выручка (списания - возвраты): " << analytics.revenue_total << "\n"
        << "  Пополнения баланса:            " << analytics.deposits_total << "\n"
        << "  Возвраты:                      " << analytics.refunds_total << "\n"
        << "  Средний чек:                   " << Money::from_double(analytics.average_check) << "\n"
        << "  Клиентов:                      " << analytics.clients_count << "\n"
        << "  Бронирований:                  " << analytics.bookings_count << "\n"
        << "  Покупок в магазине:            " << analytics.orders_count << "\n";

    section(out, "Выручка за последние 7 суток");
    for (const DailyValue& day : analytics.revenue_by_day) {
        out << "  " << timeutil::format_date(day.day) << "  " << std::fixed << std::setprecision(2) << std::setw(10)
            << day.value << " BYN\n";
    }

    section(out, "Выручка по категориям");
    write_named_values(out, analytics.revenue_by_category, "BYN");

    section(out, "Загрузка ресурсов (часы броней)");
    write_named_values(out, analytics.booked_hours_by_resource, "ч");

    section(out, "Топ клиентов по тратам");
    write_named_values(out, analytics.top_clients, "BYN");

    section(out, "Топ товаров (продано, шт.)");
    write_named_values(out, analytics.top_products, "шт.");

    section(out, "Ресурсы");
    for (const auto& resource : center.resources().values()) {
        out << "  " << resource->describe() << "\n";
    }

    section(out, "Бронирования");
    if (center.bookings().empty()) {
        out << "  (нет бронирований)\n";
    }
    for (const auto& booking : center.bookings().values()) {
        const auto client = center.users().find(booking->client_id());
        out << "  #" << booking->id() << " " << booking->title() << ", " << (client ? client->login() : "?") << ", "
            << booking->slot() << ", " << booking->price() << "\n";
    }

    section(out, "Склад");
    for (const auto& product : center.products().values()) {
        out << "  " << std::left << std::setw(34) << product->name() << std::setw(12) << product->category()
            << std::right << std::setw(10) << product->price().to_string() << "  остаток " << product->stock() << "\n";
    }

    section(out, "Последние транзакции");
    const auto entries = center.transactions().entries().values();
    const std::size_t from = entries.size() > 20 ? entries.size() - 20 : 0;
    for (std::size_t i = from; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        out << "  #" << entry->id() << " " << timeutil::format_datetime(entry->timestamp()) << " "
            << to_string(entry->type()) << " " << entry->amount() << " - " << entry->description() << "\n";
    }
    return out.str();
}

std::string build_transactions_csv(const GameCenter& center) {
    std::ostringstream out;
    out << "id;time;client;type;amount;category;description\n";
    for (const auto& [id, entry] : center.transactions().entries()) {
        const auto client = center.users().find(entry->client_id());
        out << id << ';' << timeutil::format_datetime(entry->timestamp()) << ';'
            << (client ? client->login() : std::to_string(entry->client_id())) << ';' << to_string(entry->type())
            << ';' << std::fixed << std::setprecision(2) << entry->amount().cents() / 100.0 << ';'
            << category_title(entry->category()) << ';' << entry->description() << '\n';
    }
    return out.str();
}

}  // namespace gc
