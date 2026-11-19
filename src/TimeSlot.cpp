#include "TimeSlot.h"

#include <algorithm>

#include "Exceptions.h"

namespace gc {

TimeSlot::TimeSlot(Minutes start, int duration_minutes) : start_(start), duration_(duration_minutes) {
    if (duration_minutes <= 0) {
        throw InvalidInputException("Длительность должна быть положительной");
    }
}

bool TimeSlot::overlaps(const TimeSlot& other) const {
    return start_ < other.end() && other.start_ < end();
}

int TimeSlot::night_minutes() const {
    long long total = 0;
    const long long first_day = start_ / kMinutesPerDay - 1;
    const long long last_day = end() / kMinutesPerDay;
    for (long long day = first_day; day <= last_day; ++day) {
        const long long night_begin = day * kMinutesPerDay + kNightStartMinute;
        const long long night_end = (day + 1) * kMinutesPerDay + kNightEndMinute;
        const long long low = std::max(start_, night_begin);
        const long long high = std::min(end(), night_end);
        if (high > low) {
            total += high - low;
        }
    }
    return static_cast<int>(total);
}

std::ostream& operator<<(std::ostream& out, const TimeSlot& slot) {
    return out << timeutil::format_datetime(slot.start_) << " - " << timeutil::format_datetime(slot.end())
               << " (" << slot.duration_ << " мин)";
}

}  // namespace gc
