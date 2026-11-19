#pragma once

#include <string>
#include <vector>

#include "Money.h"
#include "TimeSlot.h"

namespace gc {

// Один этап праздника: активность на одном или нескольких ресурсах.
struct TimelineItem {
    std::string activity;
    std::vector<int> resource_ids;
    TimeSlot slot;
    Money cost;
};

// Составной таймлайн праздника.
struct EventTimeline {
    std::vector<TimelineItem> items;
    Money base_total;
    int discount_percent = 0;
    Money total;

    bool empty() const { return items.empty(); }
    // Общий интервал от начала первого до конца последнего этапа.
    TimeSlot span() const;
};

// Параметры запроса к конструктору мероприятий.
struct EventRequest {
    int guests = 0;
    int avg_age = 0;
    Money budget;
    Minutes start_minute = 0;
    int slots_count = 3;
    int slot_minutes = 60;
};

}  // namespace gc
