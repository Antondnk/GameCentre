#include "EventPlanner.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>

#include "Exceptions.h"

namespace gc {

namespace {

constexpr long long kVarietyBonus = 1'000'000'000'000LL;
constexpr long long kOrderedBonus = 1'000'000'000LL;
constexpr double kMaxDiscountShare = 0.25;

using ResourcePtr = std::shared_ptr<Resource>;

// Ресурсы одного вида (kind), отсортированные по цене.
struct Group {
    std::string kind;
    int priority = 0;
    std::vector<ResourcePtr> resources;
};

struct Step {
    TimelineItem item;
    int priority;
};

struct BestResult {
    bool found = false;
    long long score = 0;
    std::vector<TimelineItem> items;
};

class Searcher {
public:
    Searcher(const GameCenter& center, const EventRequest& request, std::vector<Group> groups)
        : center_(center), request_(request), groups_(std::move(groups)) {}

    BestResult run() {
        search(0);
        return best_;
    }

private:
    std::optional<TimelineItem> build_item(const Group& group, const TimeSlot& slot) const {
        const bool is_pc = group.resources.front()->is_pc();
        const std::size_t needed = is_pc ? static_cast<std::size_t>(request_.guests) : 1;

        std::vector<int> ids;
        Money cost;
        for (const ResourcePtr& resource : group.resources) {
            if (!resource->allows_age(request_.avg_age)) {
                continue;
            }
            if (!is_pc && resource->capacity() < request_.guests) {
                continue;
            }
            if (!center_.is_resource_free(resource->id(), slot)) {
                continue;
            }
            ids.push_back(resource->id());
            cost += resource->calculate_cost(slot, request_.guests);
            if (ids.size() == needed) {
                break;
            }
        }
        if (ids.size() != needed) {
            return std::nullopt;
        }
        return TimelineItem{group.kind, ids, slot, cost};
    }

    void evaluate() {
        Money base;
        for (const Step& step : current_) {
            base += step.item.cost;
        }
        const Money total = EventPlanner::calculate_package_discount(base, request_.guests, current_.size());
        if (total > request_.budget) {
            return;
        }
        const bool ordered = std::is_sorted(current_.begin(), current_.end(), [](const Step& a, const Step& b) {
            return a.priority < b.priority;
        });
        std::set<int> categories;
        for (const Step& step : current_) {
            categories.insert(step.priority);
        }
        // Приоритет: разнообразие категорий (активность/ПК/отдых), затем порядок, затем бюджет.
        const long long score = static_cast<long long>(categories.size()) * kVarietyBonus +
                                (ordered ? kOrderedBonus : 0) + total.cents();
        if (!best_.found || score > best_.score) {
            best_.found = true;
            best_.score = score;
            best_.items.clear();
            for (const Step& step : current_) {
                best_.items.push_back(step.item);
            }
        }
    }

    void search(std::size_t position) {
        if (position == static_cast<std::size_t>(request_.slots_count)) {
            evaluate();
            return;
        }
        const TimeSlot slot(request_.start_minute + static_cast<Minutes>(position) * request_.slot_minutes,
                            request_.slot_minutes);
        for (const Group& group : groups_) {
            if (used_.count(group.kind) > 0) {
                continue;
            }
            const auto item = build_item(group, slot);
            if (!item) {
                continue;
            }
            Money running = item->cost;
            for (const Step& step : current_) {
                running += step.item.cost;
            }
            // Даже с максимальной скидкой дороже бюджета - ветку отсекаем.
            if (running * (1.0 - kMaxDiscountShare) > request_.budget) {
                continue;
            }
            used_.insert(group.kind);
            current_.push_back(Step{*item, group.priority});
            search(position + 1);
            current_.pop_back();
            used_.erase(group.kind);
        }
    }

    const GameCenter& center_;
    const EventRequest& request_;
    std::vector<Group> groups_;
    std::set<std::string> used_;
    std::vector<Step> current_;
    BestResult best_;
};

std::vector<Group> build_groups(const GameCenter& center) {
    std::map<std::string, Group> by_kind;
    for (const ResourcePtr& resource : center.resources().values()) {
        Group& group = by_kind[resource->kind()];
        group.kind = resource->kind();
        group.priority = resource->event_priority();
        group.resources.push_back(resource);
    }
    std::vector<Group> groups;
    for (auto& entry : by_kind) {
        std::sort(entry.second.resources.begin(), entry.second.resources.end(),
                  [](const ResourcePtr& a, const ResourcePtr& b) { return a->base_price() < b->base_price(); });
        groups.push_back(std::move(entry.second));
    }
    return groups;
}

void validate_request(const EventRequest& request) {
    if (request.guests < 1) {
        throw InvalidInputException("Количество гостей должно быть не меньше 1");
    }
    if (request.avg_age < 1 || request.avg_age > 120) {
        throw InvalidInputException("Средний возраст должен быть в диапазоне 1-120");
    }
    if (request.budget.cents() <= 0) {
        throw InvalidInputException("Бюджет должен быть положительным");
    }
    if (request.slots_count < 1 || request.slots_count > 5) {
        throw InvalidInputException("Количество этапов должно быть от 1 до 5");
    }
    if (request.slot_minutes < 30 || request.slot_minutes > 180) {
        throw InvalidInputException("Длительность этапа должна быть от 30 до 180 минут");
    }
}

}  // namespace

int EventPlanner::discount_percent(int guests, std::size_t activities) {
    int percent = 0;
    if (guests >= 20) {
        percent += 10;
    } else if (guests >= 10) {
        percent += 7;
    } else if (guests >= 5) {
        percent += 3;
    }
    if (activities >= 4) {
        percent += 8;
    } else if (activities == 3) {
        percent += 5;
    } else if (activities == 2) {
        percent += 2;
    }
    return percent;
}

Money EventPlanner::calculate_package_discount(const Money& base_total, int guests, std::size_t activities) {
    return base_total * (1.0 - discount_percent(guests, activities) / 100.0);
}

EventTimeline EventPlanner::generate_event_timeline(const EventRequest& request) const {
    validate_request(request);

    Searcher searcher(center_, request, build_groups(center_));
    const BestResult best = searcher.run();
    if (!best.found) {
        throw EventPlanningException(
            "Не удалось подобрать программу: нет свободных подходящих по возрасту зон на это время "
            "или бюджет слишком мал");
    }

    EventTimeline timeline;
    timeline.items = best.items;
    for (const TimelineItem& item : timeline.items) {
        timeline.base_total += item.cost;
    }
    timeline.discount_percent = discount_percent(request.guests, timeline.items.size());
    timeline.total = calculate_package_discount(timeline.base_total, request.guests, timeline.items.size());
    return timeline;
}

std::vector<std::string> EventPlanner::check_timeline_intersections(const EventTimeline& timeline) const {
    std::vector<std::string> conflicts;
    for (const TimelineItem& item : timeline.items) {
        for (const int id : item.resource_ids) {
            if (!center_.is_resource_free(id, item.slot)) {
                conflicts.push_back("\"" + center_.resources().get(id)->name() + "\" занят(а) в период " +
                                    timeutil::format_datetime(item.slot.start()) + " - " +
                                    timeutil::format_datetime(item.slot.end()));
            }
        }
    }
    return conflicts;
}

}  // namespace gc
