#include "Analytics.h"

#include <algorithm>
#include <map>

namespace gc {

namespace {

constexpr std::size_t kTopLimit = 8;

std::vector<NamedValue> sorted_top(const std::map<std::string, double>& values, std::size_t limit) {
    std::vector<NamedValue> result;
    for (const auto& [name, value] : values) {
        result.push_back(NamedValue{name, value});
    }
    std::stable_sort(result.begin(), result.end(),
                     [](const NamedValue& a, const NamedValue& b) { return a.value > b.value; });
    if (result.size() > limit) {
        result.resize(limit);
    }
    return result;
}

int hour_of(Minutes minutes) {
    return static_cast<int>((minutes % kMinutesPerDay) / 60);
}

}  // namespace

std::string category_title(const std::string& category) {
    if (category == "session") {
        return "Сеансы";
    }
    if (category == "booking") {
        return "Брони";
    }
    if (category == "event") {
        return "Праздники";
    }
    if (category == "shop") {
        return "Магазин";
    }
    if (category == "deposit") {
        return "Пополнения";
    }
    return "Прочее";
}

AnalyticsReport compute_analytics(const GameCenter& center, Minutes now, int days) {
    AnalyticsReport report;
    days = std::max(1, days);
    const long long today = now / kMinutesPerDay;

    std::map<long long, long long> cents_per_day;
    std::map<std::string, double> per_category;
    std::map<std::string, double> per_client;
    long long withdrawals_cents = 0;

    for (const auto& [id, entry] : center.transactions().entries()) {
        const long long day = entry->timestamp() / kMinutesPerDay;
        const long long cents = entry->amount().cents();
        const auto client = center.users().find(entry->client_id());
        const std::string client_name = client ? client->full_name() : std::to_string(entry->client_id());

        switch (entry->type()) {
            case TransactionType::Deposit:
                report.deposits_total += entry->amount();
                break;
            case TransactionType::Withdrawal:
                report.revenue_total += entry->amount();
                withdrawals_cents += cents;
                ++report.spend_operations;
                cents_per_day[day] += cents;
                per_category[category_title(entry->category())] += cents / 100.0;
                per_client[client_name] += cents / 100.0;
                break;
            case TransactionType::Refund:
                report.revenue_total -= entry->amount();
                report.refunds_total += entry->amount();
                cents_per_day[day] -= cents;
                per_client[client_name] -= cents / 100.0;
                break;
        }
    }
    if (report.spend_operations > 0) {
        report.average_check = withdrawals_cents / 100.0 / report.spend_operations;
    }

    for (long long day = today - days + 1; day <= today; ++day) {
        const auto found = cents_per_day.find(day);
        report.revenue_by_day.push_back(DailyValue{day, found == cents_per_day.end() ? 0.0 : found->second / 100.0});
    }
    report.revenue_by_category = sorted_top(per_category, kTopLimit);
    report.top_clients = sorted_top(per_client, kTopLimit);

    // Загрузка ресурсов и распределение броней по часам.
    std::map<int, double> hours_per_resource;
    for (const auto& booking : center.bookings().values()) {
        ++report.bookings_count;
        if (const auto event = std::dynamic_pointer_cast<EventBooking>(booking)) {
            for (const TimelineItem& item : event->timeline().items) {
                for (const int resource_id : item.resource_ids) {
                    hours_per_resource[resource_id] += item.slot.hours();
                }
                ++report.bookings_by_hour[static_cast<std::size_t>(hour_of(item.slot.start()))];
            }
        } else {
            hours_per_resource[booking->resource_id()] += booking->slot().hours();
            ++report.bookings_by_hour[static_cast<std::size_t>(hour_of(booking->slot().start()))];
        }
    }
    std::map<std::string, double> hours_by_name;
    for (const auto& [resource_id, hours] : hours_per_resource) {
        const auto resource = center.resources().find(resource_id);
        hours_by_name[resource ? resource->name() : "(удалён)"] += hours;
    }
    report.booked_hours_by_resource = sorted_top(hours_by_name, kTopLimit);

    // Продажи магазина.
    std::map<std::string, double> units_per_product;
    for (const auto& [id, order] : center.orders()) {
        units_per_product[order->product_name()] += order->quantity();
        ++report.orders_count;
    }
    report.top_products = sorted_top(units_per_product, kTopLimit);

    for (const auto& user : center.users().values()) {
        if (user->get_role() == Role::Client) {
            ++report.clients_count;
        }
    }
    return report;
}

}  // namespace gc
