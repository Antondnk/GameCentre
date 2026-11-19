#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Money.h"
#include "TimeSlot.h"
#include "Timeline.h"

namespace gc {

// Бронирование одного ресурса.
class Booking {
public:
    Booking(int id, int client_id, int resource_id, TimeSlot slot, int guests, Money price);
    virtual ~Booking() = default;

    int id() const { return id_; }
    int client_id() const { return client_id_; }
    int resource_id() const { return resource_id_; }
    const TimeSlot& slot() const { return slot_; }
    int guests() const { return guests_; }
    const Money& price() const { return price_; }

    // Блокирует ли бронь ресурс в заданном интервале.
    virtual bool conflicts_with(int resource_id, const TimeSlot& slot) const;
    virtual std::vector<int> resource_ids() const { return {resource_id_}; }
    virtual std::string title() const { return "Бронь ресурса"; }
    virtual std::string serialize() const;

protected:
    std::vector<std::string> base_fields(const std::string& tag) const;

private:
    int id_;
    int client_id_;
    int resource_id_;
    TimeSlot slot_;
    int guests_;
    Money price_;
};

// Масштабное мероприятие: блокирует сразу несколько ресурсов по таймлайну.
class EventBooking : public Booking {
public:
    EventBooking(int id, int client_id, std::string event_name, int guests, EventTimeline timeline);

    const std::string& event_name() const { return event_name_; }
    const EventTimeline& timeline() const { return timeline_; }

    bool conflicts_with(int resource_id, const TimeSlot& slot) const override;
    std::vector<int> resource_ids() const override;
    std::string title() const override { return "Мероприятие \"" + event_name_ + "\""; }
    std::string serialize() const override;

private:
    std::string event_name_;
    EventTimeline timeline_;
};

std::shared_ptr<Booking> make_booking(const std::vector<std::string>& fields);

}  // namespace gc
