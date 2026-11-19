#include "Resource.h"

#include <cmath>
#include <sstream>

#include "Exceptions.h"
#include "Utils.h"

namespace gc {

std::string to_string(ResourceStatus status) {
    switch (status) {
        case ResourceStatus::Free:
            return "Свободен";
        case ResourceStatus::Occupied:
            return "Занят";
        case ResourceStatus::Booked:
            return "Забронирован";
        case ResourceStatus::Maintenance:
            return "На обслуживании";
    }
    return "Неизвестно";
}

// ---------------------------------------------------------------- Resource

Resource::Resource(int id, std::string name, Money base_price, int min_age, int max_age, int capacity)
    : id_(id),
      name_(util::sanitize_field(std::move(name))),
      base_price_(base_price),
      min_age_(min_age),
      max_age_(max_age),
      capacity_(capacity) {
    if (min_age < 0 || max_age < min_age) {
        throw InvalidInputException("Некорректный возрастной диапазон");
    }
    if (capacity < 1) {
        throw InvalidInputException("Вместимость должна быть не меньше 1");
    }
    if (base_price.cents() < 0) {
        throw InvalidInputException("Цена не может быть отрицательной");
    }
}

void Resource::occupy_until(Minutes until) {
    status_ = ResourceStatus::Occupied;
    occupied_until_ = until;
}

void Resource::release() {
    status_ = ResourceStatus::Free;
    occupied_until_ = 0;
}

std::string Resource::describe() const {
    std::ostringstream out;
    out << "[" << id_ << "] " << name_ << " (" << kind() << "), " << base_price_ << " " << price_unit()
        << ", возраст " << min_age_ << "-" << max_age_ << ", вместимость " << capacity_ << ", статус: "
        << to_string(status_);
    return out.str();
}

std::string Resource::serialize() const {
    std::vector<std::string> fields = {type_tag(),
                                       std::to_string(id_),
                                       name_,
                                       std::to_string(base_price_.cents()),
                                       std::to_string(min_age_),
                                       std::to_string(max_age_),
                                       std::to_string(capacity_),
                                       std::to_string(static_cast<int>(status_)),
                                       std::to_string(occupied_until_)};
    for (const std::string& extra : extra_fields()) {
        fields.push_back(extra);
    }
    return util::join_values(fields, '|');
}

std::ostream& operator<<(std::ostream& out, const Resource& resource) {
    return out << resource.describe();
}

// ------------------------------------------------------------------ PCZone

PCZone::PCZone(int id, std::string name, Money price_per_hour, PcTier tier, std::string cpu,
               std::string gpu, int ram_gb)
    : Resource(id, std::move(name), price_per_hour, 6, 120, 1),
      tier_(tier),
      cpu_(util::sanitize_field(std::move(cpu))),
      gpu_(util::sanitize_field(std::move(gpu))),
      ram_gb_(ram_gb) {}

std::string PCZone::get_pc_specs() const {
    std::ostringstream out;
    out << "CPU: " << cpu_ << ", GPU: " << gpu_ << ", RAM: " << ram_gb_ << " ГБ";
    return out.str();
}

Money PCZone::calculate_cost(const TimeSlot& slot, int /*guests*/) const {
    const double per_minute = static_cast<double>(base_price().cents()) / 60.0;
    const int night = slot.night_minutes();
    const int day = slot.duration() - night;
    const double cents = per_minute * (day + night * kNightPcCoefficient) * tier_coefficient();
    return Money(std::llround(cents));
}

std::string PCZone::describe() const {
    return Resource::describe() + ", " + (tier_ == PcTier::Vip ? "VIP" : "Стандарт") + "; " + get_pc_specs();
}

std::vector<std::string> PCZone::extra_fields() const {
    return {std::to_string(static_cast<int>(tier_)), cpu_, gpu_, std::to_string(ram_gb_)};
}

// -------------------------------------------------------------- ActiveZone

Money ActiveZone::calculate_cost(const TimeSlot& slot, int guests) const {
    if (guests < 1) {
        throw InvalidInputException("Количество участников должно быть не меньше 1");
    }
    return base_price() * (guests * slot.hours());
}

LaserTagArena::LaserTagArena(int id, std::string name, Money price_per_person_hour, int min_age,
                             int max_age, int capacity, int players_per_team)
    : ActiveZone(id, std::move(name), price_per_person_hour, min_age, max_age, capacity),
      players_per_team_(players_per_team) {}

std::string LaserTagArena::describe() const {
    return ActiveZone::describe() + ", игроков в команде: " + std::to_string(players_per_team_);
}

std::vector<std::string> LaserTagArena::extra_fields() const {
    return {std::to_string(players_per_team_)};
}

RopePark::RopePark(int id, std::string name, Money price_per_person_hour, int min_age, int max_age,
                   int capacity, int max_height_m)
    : ActiveZone(id, std::move(name), price_per_person_hour, min_age, max_age, capacity),
      max_height_m_(max_height_m) {}

std::string RopePark::describe() const {
    return ActiveZone::describe() + ", максимальная высота: " + std::to_string(max_height_m_) + " м";
}

std::vector<std::string> RopePark::extra_fields() const {
    return {std::to_string(max_height_m_)};
}

ClimbingWall::ClimbingWall(int id, std::string name, Money price_per_person_hour, int min_age,
                           int max_age, int capacity, int wall_height_m)
    : ActiveZone(id, std::move(name), price_per_person_hour, min_age, max_age, capacity),
      wall_height_m_(wall_height_m) {}

std::string ClimbingWall::describe() const {
    return ActiveZone::describe() + ", высота стенки: " + std::to_string(wall_height_m_) + " м";
}

std::vector<std::string> ClimbingWall::extra_fields() const {
    return {std::to_string(wall_height_m_)};
}

// -------------------------------------------------------------- RelaxZone

RelaxZone::RelaxZone(int id, std::string name, Money price_per_hour, int capacity)
    : Resource(id, std::move(name), price_per_hour, 0, 120, capacity) {}

Money RelaxZone::calculate_cost(const TimeSlot& slot, int /*guests*/) const {
    return base_price() * slot.hours();
}

// ------------------------------------------------------------------ Фабрика

namespace {

void require_fields(const std::vector<std::string>& fields, std::size_t count) {
    if (fields.size() < count) {
        throw FileFormatException("Недостаточно полей в записи ресурса");
    }
}

}  // namespace

std::shared_ptr<Resource> make_resource(const std::vector<std::string>& fields) {
    constexpr std::size_t kCommon = 9;
    require_fields(fields, kCommon);

    const std::string& tag = fields[0];
    const int id = util::parse_int(fields[1], "id");
    const std::string& name = fields[2];
    const Money price(util::parse_long_long(fields[3], "price"));
    const int min_age = util::parse_int(fields[4], "min_age");
    const int max_age = util::parse_int(fields[5], "max_age");
    const int capacity = util::parse_int(fields[6], "capacity");
    const int status = util::parse_int(fields[7], "status");
    const Minutes occupied_until = util::parse_long_long(fields[8], "occupied_until");

    std::shared_ptr<Resource> resource;
    if (tag == "PC") {
        require_fields(fields, kCommon + 4);
        const int tier = util::parse_int(fields[9], "tier");
        resource = std::make_shared<PCZone>(id, name, price, tier == 1 ? PcTier::Vip : PcTier::Standard,
                                            fields[10], fields[11], util::parse_int(fields[12], "ram"));
    } else if (tag == "LASER") {
        require_fields(fields, kCommon + 1);
        resource = std::make_shared<LaserTagArena>(id, name, price, min_age, max_age, capacity,
                                                   util::parse_int(fields[9], "players_per_team"));
    } else if (tag == "ROPE") {
        require_fields(fields, kCommon + 1);
        resource = std::make_shared<RopePark>(id, name, price, min_age, max_age, capacity,
                                              util::parse_int(fields[9], "max_height"));
    } else if (tag == "WALL") {
        require_fields(fields, kCommon + 1);
        resource = std::make_shared<ClimbingWall>(id, name, price, min_age, max_age, capacity,
                                                  util::parse_int(fields[9], "wall_height"));
    } else if (tag == "RELAX") {
        resource = std::make_shared<RelaxZone>(id, name, price, capacity);
    } else {
        throw FileFormatException("Неизвестный тип ресурса: " + tag);
    }

    resource->set_status(static_cast<ResourceStatus>(status));
    if (resource->status() == ResourceStatus::Occupied) {
        resource->occupy_until(occupied_until);
    }
    return resource;
}

}  // namespace gc
