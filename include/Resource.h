#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "Money.h"
#include "TimeSlot.h"

namespace gc {

enum class ResourceStatus { Free = 0, Occupied = 1, Booked = 2, Maintenance = 3 };

std::string to_string(ResourceStatus status);

// Ночной коэффициент тарифа ПК (ночью дешевле).
constexpr double kNightPcCoefficient = 0.7;

// Абстрактный базовый класс любого ресурса игрового центра.
class Resource {
public:
    Resource(int id, std::string name, Money base_price, int min_age, int max_age, int capacity);
    virtual ~Resource() = default;

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    Money base_price() const { return base_price_; }
    int min_age() const { return min_age_; }
    int max_age() const { return max_age_; }
    int capacity() const { return capacity_; }

    ResourceStatus status() const { return status_; }
    void set_status(ResourceStatus status) { status_ = status; }
    Minutes occupied_until() const { return occupied_until_; }
    void set_base_price(Money price);
    void occupy_until(Minutes until);
    void release();

    bool is_available() const { return status_ != ResourceStatus::Maintenance; }
    // Входит ли возраст в диапазон [min_age; max_age].
    bool allows_age(int age) const { return age >= min_age_ && age <= max_age_; }

    // Виртуальные функции, специфичные для каждого типа ресурса.
    virtual std::string kind() const = 0;
    virtual std::string type_tag() const = 0;
    virtual std::string price_unit() const = 0;
    virtual Money calculate_cost(const TimeSlot& slot, int guests) const = 0;
    virtual bool is_pc() const { return false; }
    // Порядок в программе праздника: 0 - активность, 1 - ПК, 2 - отдых.
    virtual int event_priority() const = 0;
    virtual std::string describe() const;

    std::string serialize() const;

    // Дружественная функция: вывод описания ресурса в поток.
    friend std::ostream& operator<<(std::ostream& out, const Resource& resource);

protected:
    virtual std::vector<std::string> extra_fields() const { return {}; }

private:
    int id_;
    std::string name_;
    Money base_price_;
    int min_age_;
    int max_age_;
    int capacity_;
    ResourceStatus status_ = ResourceStatus::Free;
    Minutes occupied_until_ = 0;
};

enum class PcTier { Standard, Vip };

// Киберспортивный компьютер (одно рабочее место).
class PCZone : public Resource {
public:
    PCZone(int id, std::string name, Money price_per_hour, PcTier tier, std::string cpu, std::string gpu,
           int ram_gb);

    PcTier tier() const { return tier_; }
    double tier_coefficient() const { return tier_ == PcTier::Vip ? 1.5 : 1.0; }
    // Характеристики оборудования (get_pc_specs).
    std::string get_pc_specs() const;

    std::string kind() const override { return "ПК-зона"; }
    std::string type_tag() const override { return "PC"; }
    std::string price_unit() const override { return "за час"; }
    // Стоимость сеанса с учётом тарифа (Standard/VIP) и ночного коэффициента (calculate_pc_cost).
    Money calculate_cost(const TimeSlot& slot, int guests) const override;
    bool is_pc() const override { return true; }
    int event_priority() const override { return 1; }
    std::string describe() const override;

protected:
    std::vector<std::string> extra_fields() const override;

private:
    PcTier tier_;
    std::string cpu_;
    std::string gpu_;
    int ram_gb_;
};

// Абстрактная активная зона: цена считается за человека в час.
class ActiveZone : public Resource {
public:
    using Resource::Resource;

    std::string price_unit() const override { return "за чел./час"; }
    Money calculate_cost(const TimeSlot& slot, int guests) const override;
    int event_priority() const override { return 0; }
};

class LaserTagArena : public ActiveZone {
public:
    LaserTagArena(int id, std::string name, Money price_per_person_hour, int min_age, int max_age,
                  int capacity, int players_per_team);

    std::string kind() const override { return "Лазертаг"; }
    std::string type_tag() const override { return "LASER"; }
    std::string describe() const override;

protected:
    std::vector<std::string> extra_fields() const override;

private:
    int players_per_team_;
};

class RopePark : public ActiveZone {
public:
    RopePark(int id, std::string name, Money price_per_person_hour, int min_age, int max_age, int capacity,
             int max_height_m);

    std::string kind() const override { return "Верёвочный городок"; }
    std::string type_tag() const override { return "ROPE"; }
    std::string describe() const override;

protected:
    std::vector<std::string> extra_fields() const override;

private:
    int max_height_m_;
};

class ClimbingWall : public ActiveZone {
public:
    ClimbingWall(int id, std::string name, Money price_per_person_hour, int min_age, int max_age,
                 int capacity, int wall_height_m);

    std::string kind() const override { return "Скалодром"; }
    std::string type_tag() const override { return "WALL"; }
    std::string describe() const override;

protected:
    std::vector<std::string> extra_fields() const override;

private:
    int wall_height_m_;
};

// Зона отдыха (кафе / банкетный зал): фиксированная цена за час.
class RelaxZone : public Resource {
public:
    RelaxZone(int id, std::string name, Money price_per_hour, int capacity);

    std::string kind() const override { return "Зона отдыха"; }
    std::string type_tag() const override { return "RELAX"; }
    std::string price_unit() const override { return "за час"; }
    Money calculate_cost(const TimeSlot& slot, int guests) const override;
    int event_priority() const override { return 2; }
};

// VR-арена: цена за человека в час.
class VRZone : public ActiveZone {
public:
    VRZone(int id, std::string name, Money price_per_person_hour, int min_age, int max_age, int capacity,
           int headsets);

    std::string kind() const override { return "VR-арена"; }
    std::string type_tag() const override { return "VR"; }
    std::string describe() const override;

protected:
    std::vector<std::string> extra_fields() const override;

private:
    int headsets_;
};

// Игровые консоли (PlayStation / Xbox): фиксированная цена за час на всю зону.
class ConsoleZone : public Resource {
public:
    ConsoleZone(int id, std::string name, Money price_per_hour, int capacity, std::string model);

    std::string kind() const override { return "Консоли"; }
    std::string type_tag() const override { return "CONSOLE"; }
    std::string price_unit() const override { return "за час"; }
    Money calculate_cost(const TimeSlot& slot, int guests) const override;
    int event_priority() const override { return 1; }
    std::string describe() const override;

protected:
    std::vector<std::string> extra_fields() const override;

private:
    std::string model_;
};

// Бильярдный стол: цена за час.
class BilliardsTable : public Resource {
public:
    BilliardsTable(int id, std::string name, Money price_per_hour, int capacity);

    std::string kind() const override { return "Бильярд"; }
    std::string type_tag() const override { return "BILLIARD"; }
    std::string price_unit() const override { return "за час"; }
    Money calculate_cost(const TimeSlot& slot, int guests) const override;
    int event_priority() const override { return 1; }
};

// Фабрика: восстановление ресурса из полей строки файла.
std::shared_ptr<Resource> make_resource(const std::vector<std::string>& fields);

}  // namespace gc
