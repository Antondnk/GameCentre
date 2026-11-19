#pragma once

#include <string>
#include <vector>

#include "GameCenter.h"
#include "Timeline.h"

namespace gc {

// Конструктор мероприятий («киллер-фича»): подбирает последовательность
// активностей (лазертаг -> ПК-турнир -> отдых и т.п.) под гостей, возраст и бюджет.
class EventPlanner {
public:
    explicit EventPlanner(const GameCenter& center) : center_(center) {}

    // Перебор последовательностей разных видов активностей; выбирается самая
    // "правильная" по порядку (активность -> ПК -> отдых) и максимально использующая бюджет.
    // Бросает EventPlanningException, если подобрать программу не удалось.
    EventTimeline generate_event_timeline(const EventRequest& request) const;

    // Возвращает описания конфликтов (пустой список - таймлайн свободен).
    std::vector<std::string> check_timeline_intersections(const EventTimeline& timeline) const;

    // Процент скидки: зависит от числа гостей и числа активностей.
    static int discount_percent(int guests, std::size_t activities);
    // Итоговая цена пакета со скидкой.
    static Money calculate_package_discount(const Money& base_total, int guests, std::size_t activities);

private:
    const GameCenter& center_;
};

}  // namespace gc
