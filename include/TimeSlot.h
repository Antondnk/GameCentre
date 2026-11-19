#pragma once

#include <ostream>

#include "TimeUtils.h"

namespace gc {

// Временной интервал [start, start + duration) в минутах.
class TimeSlot {
public:
    // Бросает InvalidInputException, если длительность <= 0.
    TimeSlot(Minutes start, int duration_minutes);

    Minutes start() const { return start_; }
    int duration() const { return duration_; }
    Minutes end() const { return start_ + duration_; }
    double hours() const { return duration_ / 60.0; }

    bool overlaps(const TimeSlot& other) const;
    // Сколько минут интервала попадает в ночное окно (23:00-06:00).
    int night_minutes() const;
    bool touches_night() const { return night_minutes() > 0; }

    bool operator==(const TimeSlot& other) const {
        return start_ == other.start_ && duration_ == other.duration_;
    }

    // Дружественная функция: вывод интервала в поток.
    friend std::ostream& operator<<(std::ostream& out, const TimeSlot& slot);

private:
    Minutes start_;
    int duration_;
};

}  // namespace gc
