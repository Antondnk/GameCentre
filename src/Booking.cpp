#include "Booking.h"

#include <algorithm>

#include "Exceptions.h"
#include "Utils.h"

namespace gc {

TimeSlot EventTimeline::span() const {
    if (items.empty()) {
        throw InvalidInputException("Пустой таймлайн мероприятия");
    }
    const Minutes begin = items.front().slot.start();
    const Minutes finish = items.back().slot.end();
    return TimeSlot(begin, static_cast<int>(finish - begin));
}

// ------------------------------------------------------------------ Booking

Booking::Booking(int id, int client_id, int resource_id, TimeSlot slot, int guests, Money price)
    : id_(id), client_id_(client_id), resource_id_(resource_id), slot_(slot), guests_(guests), price_(price) {}

bool Booking::conflicts_with(int resource_id, const TimeSlot& slot) const {
    return resource_id == resource_id_ && slot_.overlaps(slot);
}

std::vector<std::string> Booking::base_fields(const std::string& tag) const {
    return {tag,
            std::to_string(id_),
            std::to_string(client_id_),
            std::to_string(resource_id_),
            std::to_string(slot_.start()),
            std::to_string(slot_.duration()),
            std::to_string(guests_),
            std::to_string(price_.cents())};
}

std::string Booking::serialize() const {
    return util::join_values(base_fields("B"), '|');
}

// -------------------------------------------------------------- EventBooking

EventBooking::EventBooking(int id, int client_id, std::string event_name, int guests, EventTimeline timeline)
    : Booking(id, client_id, -1, timeline.span(), guests, timeline.total),
      event_name_(util::sanitize_field(std::move(event_name))),
      timeline_(std::move(timeline)) {}

bool EventBooking::conflicts_with(int resource_id, const TimeSlot& slot) const {
    return std::any_of(timeline_.items.begin(), timeline_.items.end(), [&](const TimelineItem& item) {
        const auto& ids = item.resource_ids;
        return std::find(ids.begin(), ids.end(), resource_id) != ids.end() && item.slot.overlaps(slot);
    });
}

std::vector<int> EventBooking::resource_ids() const {
    std::vector<int> ids;
    for (const TimelineItem& item : timeline_.items) {
        ids.insert(ids.end(), item.resource_ids.begin(), item.resource_ids.end());
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::string EventBooking::serialize() const {
    std::vector<std::string> encoded_items;
    for (const TimelineItem& item : timeline_.items) {
        const std::vector<std::string> parts = {item.activity, std::to_string(item.slot.start()),
                                                std::to_string(item.slot.duration()),
                                                std::to_string(item.cost.cents()),
                                                util::join_values(item.resource_ids, ':')};
        encoded_items.push_back(util::join_values(parts, ','));
    }
    std::vector<std::string> fields = base_fields("E");
    fields.push_back(event_name_);
    fields.push_back(std::to_string(timeline_.discount_percent));
    fields.push_back(util::join_values(encoded_items, ';'));
    return util::join_values(fields, '|');
}

// ------------------------------------------------------------------ Фабрика

std::shared_ptr<Booking> make_booking(const std::vector<std::string>& fields) {
    constexpr std::size_t kBase = 8;
    if (fields.size() < kBase) {
        throw FileFormatException("Недостаточно полей в записи бронирования");
    }
    const int id = util::parse_int(fields[1], "id");
    const int client_id = util::parse_int(fields[2], "client_id");
    const int resource_id = util::parse_int(fields[3], "resource_id");
    const TimeSlot slot(util::parse_long_long(fields[4], "start"), util::parse_int(fields[5], "duration"));
    const int guests = util::parse_int(fields[6], "guests");
    const Money price(util::parse_long_long(fields[7], "price"));

    if (fields[0] == "B") {
        return std::make_shared<Booking>(id, client_id, resource_id, slot, guests, price);
    }
    if (fields[0] == "E") {
        if (fields.size() < kBase + 3) {
            throw FileFormatException("Недостаточно полей в записи мероприятия");
        }
        EventTimeline timeline;
        timeline.discount_percent = util::parse_int(fields[9], "discount");
        timeline.total = price;
        for (const std::string& encoded : util::split(fields[10], ';')) {
            const std::vector<std::string> parts = util::split(encoded, ',');
            if (parts.size() < 5) {
                throw FileFormatException("Некорректный этап мероприятия");
            }
            const Money cost(util::parse_long_long(parts[3], "item_cost"));
            timeline.items.push_back(TimelineItem{
                parts[0], util::split_values<int>(parts[4], ':'),
                TimeSlot(util::parse_long_long(parts[1], "item_start"), util::parse_int(parts[2], "item_duration")),
                cost});
            timeline.base_total += cost;
        }
        return std::make_shared<EventBooking>(id, client_id, fields[8], guests, std::move(timeline));
    }
    throw FileFormatException("Неизвестный тип бронирования: " + fields[0]);
}

}  // namespace gc
