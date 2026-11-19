#include "GameCenter.h"

#include <algorithm>
#include <filesystem>

#include "EventPlanner.h"
#include "Exceptions.h"
#include "TimeUtils.h"
#include "Utils.h"

namespace gc {

namespace {

constexpr int kMinPasswordLength = 4;
constexpr int kAdultAge = 18;

std::string file_path(const std::string& directory, const std::string& name) {
    return (std::filesystem::path(directory) / name).string();
}

}  // namespace

// ------------------------------------------------------------ Пользователи

std::shared_ptr<User> GameCenter::find_user_by_login(const std::string& login) const {
    const auto found = users_.find_all([&](const User& user) { return user.login() == login; });
    return found.empty() ? nullptr : found.front();
}

void GameCenter::require_free_login(const std::string& login) const {
    if (find_user_by_login(util::sanitize_field(login))) {
        throw DuplicateItemException("Логин \"" + login + "\" уже занят");
    }
}

std::shared_ptr<User> GameCenter::authenticate(const std::string& login, const std::string& password) const {
    const auto user = find_user_by_login(login);
    if (!user || !user->check_password(password)) {
        throw AuthenticationException("Неверный логин или пароль");
    }
    return user;
}

std::shared_ptr<Client> GameCenter::register_client(const std::string& login, const std::string& password,
                                                    const std::string& full_name, long long birth_days) {
    require_free_login(login);
    if (static_cast<int>(password.size()) < kMinPasswordLength) {
        throw InvalidInputException("Пароль должен содержать не менее 4 символов");
    }
    const std::string salt = User::generate_salt();
    auto client = std::make_shared<Client>(users_.next_id(), login, salt, User::make_hash(salt, password),
                                           full_name, birth_days, Money(), 0);
    users_.add_item(client);
    return client;
}

std::shared_ptr<User> GameCenter::create_staff(Role role, const std::string& login,
                                               const std::string& password, const std::string& full_name) {
    require_free_login(login);
    if (static_cast<int>(password.size()) < kMinPasswordLength) {
        throw InvalidInputException("Пароль должен содержать не менее 4 символов");
    }
    const std::string salt = User::generate_salt();
    const std::string hash = User::make_hash(salt, password);
    std::shared_ptr<User> user;
    if (role == Role::Owner) {
        user = std::make_shared<Owner>(users_.next_id(), login, salt, hash, full_name);
    } else if (role == Role::Administrator) {
        user = std::make_shared<Administrator>(users_.next_id(), login, salt, hash, full_name);
    } else {
        throw InvalidInputException("Клиентов нужно создавать через register_client");
    }
    users_.add_item(user);
    return user;
}

std::shared_ptr<Client> GameCenter::get_client(int client_id) const {
    auto client = std::dynamic_pointer_cast<Client>(users_.get(client_id));
    if (!client) {
        throw ItemNotFoundException("Пользователь " + std::to_string(client_id) + " не является клиентом");
    }
    return client;
}

Role GameCenter::get_role(int user_id) const {
    return users_.get(user_id)->get_role();
}

// ----------------------------------------------------------------- Финансы

void GameCenter::deposit(int client_id, const Money& amount, Minutes now) {
    if (amount.cents() <= 0) {
        throw InvalidInputException("Сумма пополнения должна быть положительной");
    }
    get_client(client_id)->add_funds(amount);
    log_.record(client_id, now, TransactionType::Deposit, amount, "Пополнение баланса");
}

void GameCenter::withdraw(int client_id, const Money& amount, const std::string& reason, Minutes now) {
    if (amount.cents() <= 0) {
        throw InvalidInputException("Сумма списания должна быть положительной");
    }
    get_client(client_id)->charge(amount);  // бросает InsufficientFundsException
    log_.record(client_id, now, TransactionType::Withdrawal, amount, reason);
}

Money GameCenter::quote(const Client& client, const Resource& resource, const TimeSlot& slot,
                        int guests) const {
    const Money base = resource.calculate_cost(slot, guests);
    return base * (1.0 - client.discount_percent() / 100.0);
}

// ----------------------------------------------------------------- Ресурсы

void GameCenter::add_resource(std::shared_ptr<Resource> resource) {
    resources_.add_item(std::move(resource));
}

void GameCenter::remove_resource(int resource_id) {
    for (const auto& booking : bookings_.values()) {
        const auto ids = booking->resource_ids();
        if (std::find(ids.begin(), ids.end(), resource_id) != ids.end()) {
            throw ResourceUnavailableException("Нельзя удалить ресурс: есть связанные бронирования");
        }
    }
    resources_.remove_item(resource_id);
}

ResourceStatus GameCenter::get_device_status(int resource_id, Minutes now) const {
    const auto resource = resources_.get(resource_id);
    if (resource->status() == ResourceStatus::Free) {
        const TimeSlot probe(now, 1);
        for (const auto& booking : bookings_.values()) {
            if (booking->conflicts_with(resource_id, probe)) {
                return ResourceStatus::Booked;
            }
        }
    }
    return resource->status();
}

void GameCenter::set_device_status(int resource_id, ResourceStatus status) {
    resources_.get(resource_id)->set_status(status);
}

void GameCenter::set_zone_maintenance(int resource_id, bool enabled) {
    const auto resource = resources_.get(resource_id);
    if (enabled) {
        resource->set_status(ResourceStatus::Maintenance);
    } else if (resource->status() == ResourceStatus::Maintenance) {
        resource->release();
    }
}

void GameCenter::refresh_statuses(Minutes now) {
    for (const auto& resource : resources_.values()) {
        if (resource->status() == ResourceStatus::Occupied && resource->occupied_until() <= now) {
            resource->release();
        }
    }
}

bool GameCenter::is_resource_free(int resource_id, const TimeSlot& slot, int ignore_booking_id) const {
    const auto resource = resources_.get(resource_id);
    if (!resource->is_available()) {
        return false;
    }
    if (resource->status() == ResourceStatus::Occupied && slot.start() < resource->occupied_until()) {
        return false;
    }
    for (const auto& [id, booking] : bookings_) {
        if (id != ignore_booking_id && booking->conflicts_with(resource_id, slot)) {
            return false;
        }
    }
    return true;
}

// ------------------------------------------------------ Возрастные правила

void GameCenter::check_age_validity(const Client& client, const TimeSlot& slot) {
    const int age = client.age_on(slot.start() / kMinutesPerDay);
    if (age < kAdultAge && slot.touches_night()) {
        throw AgeValidationException("Клиентам младше 18 лет нельзя находиться в центре с 23:00 до 06:00");
    }
}

void GameCenter::validate_zone_age(int age, const Resource& resource) {
    if (!resource.allows_age(age)) {
        throw AgeValidationException("Возраст " + std::to_string(age) + " не подходит для \"" +
                                     resource.name() + "\" (допустимо " + std::to_string(resource.min_age()) +
                                     "-" + std::to_string(resource.max_age()) + ")");
    }
}

// ------------------------------------------------------ Сеансы и брони

void GameCenter::require_future(const TimeSlot& slot, Minutes now) {
    if (slot.start() < now) {
        throw InvalidInputException("Время начала уже прошло");
    }
}

Money GameCenter::start_session(int client_id, int resource_id, int minutes, int guests, Minutes now) {
    const auto client = get_client(client_id);
    const auto resource = resources_.get(resource_id);
    const TimeSlot slot(now, minutes);

    if (guests < 1 || guests > resource->capacity()) {
        throw InvalidInputException("Количество участников должно быть от 1 до " +
                                    std::to_string(resource->capacity()));
    }
    if (!resource->is_available()) {
        throw ResourceUnavailableException("Ресурс \"" + resource->name() + "\" на обслуживании");
    }
    validate_zone_age(client->age_on(now / kMinutesPerDay), *resource);
    check_age_validity(*client, slot);
    if (!is_resource_free(resource_id, slot)) {
        throw BookingConflictException("Ресурс \"" + resource->name() + "\" занят или забронирован");
    }

    const Money cost = quote(*client, *resource, slot, guests);
    withdraw(client_id, cost, "Сеанс: " + resource->name(), now);
    resource->occupy_until(slot.end());
    return cost;
}

std::shared_ptr<Booking> GameCenter::create_booking(int client_id, int resource_id, const TimeSlot& slot,
                                                    int guests, Minutes now) {
    const auto client = get_client(client_id);
    const auto resource = resources_.get(resource_id);
    require_future(slot, now);

    if (guests < 1 || guests > resource->capacity()) {
        throw InvalidInputException("Количество участников должно быть от 1 до " +
                                    std::to_string(resource->capacity()));
    }
    if (!resource->is_available()) {
        throw ResourceUnavailableException("Ресурс \"" + resource->name() + "\" на обслуживании");
    }
    validate_zone_age(client->age_on(slot.start() / kMinutesPerDay), *resource);
    check_age_validity(*client, slot);
    if (!is_resource_free(resource_id, slot)) {
        throw BookingConflictException("Ресурс \"" + resource->name() + "\" уже занят в это время");
    }

    const Money price = quote(*client, *resource, slot, guests);
    withdraw(client_id, price, "Бронь: " + resource->name(), now);
    auto booking = std::make_shared<Booking>(bookings_.next_id(), client_id, resource_id, slot, guests, price);
    bookings_.add_item(booking);
    return booking;
}

Money GameCenter::cancel_booking(int booking_id, int requester_id, Minutes now) {
    const auto booking = bookings_.get(booking_id);
    if (requester_id >= 0 && booking->client_id() != requester_id) {
        throw AccessDeniedException("Нельзя отменить чужую бронь");
    }
    const Minutes minutes_left = booking->slot().start() - now;
    if (minutes_left <= 0) {
        throw InvalidInputException("Бронь уже началась, отмена невозможна");
    }

    // Правила возврата: >= 24 ч - 100%, >= 2 ч - 50%, иначе без возврата.
    double refund_share = 0.0;
    if (minutes_left >= 24 * 60) {
        refund_share = 1.0;
    } else if (minutes_left >= 2 * 60) {
        refund_share = 0.5;
    }
    const Money refund = booking->price() * refund_share;
    if (refund.cents() > 0) {
        get_client(booking->client_id())->add_funds(refund);
        log_.record(booking->client_id(), now, TransactionType::Refund, refund,
                    "Возврат за отмену брони #" + std::to_string(booking_id));
    }
    bookings_.remove_item(booking_id);
    return refund;
}

EventTimeline GameCenter::generate_event_timeline(const EventRequest& request) const {
    return EventPlanner(*this).generate_event_timeline(request);
}

std::shared_ptr<EventBooking> GameCenter::create_event_booking(int client_id, const std::string& event_name,
                                                               const EventTimeline& timeline, int guests,
                                                               int avg_age, Minutes now) {
    const auto client = get_client(client_id);
    if (timeline.empty()) {
        throw InvalidInputException("Таймлайн мероприятия пуст");
    }
    const TimeSlot span = timeline.span();
    require_future(span, now);
    if (client->age_on(span.start() / kMinutesPerDay) < kAdultAge) {
        throw AgeValidationException("Организатор мероприятия должен быть совершеннолетним");
    }
    check_age_validity(*client, span);

    for (const TimelineItem& item : timeline.items) {
        for (const int id : item.resource_ids) {
            validate_zone_age(avg_age, *resources_.get(id));
        }
    }
    const auto conflicts = EventPlanner(*this).check_timeline_intersections(timeline);
    if (!conflicts.empty()) {
        throw BookingConflictException("Конфликт расписания: " + conflicts.front());
    }

    withdraw(client_id, timeline.total, "Мероприятие: " + event_name, now);
    auto booking = std::make_shared<EventBooking>(bookings_.next_id(), client_id, event_name, guests, timeline);
    bookings_.add_item(booking);
    return booking;
}

// ------------------------------------------------------------------ Хранение

void GameCenter::save_all(const std::string& directory) const {
    std::filesystem::create_directories(directory);
    users_.save_to_file(file_path(directory, "users.txt"));
    resources_.save_to_file(file_path(directory, "resources.txt"));
    bookings_.save_to_file(file_path(directory, "bookings.txt"));
    log_.entries().save_to_file(file_path(directory, "transactions.txt"));
}

bool GameCenter::load_all(const std::string& directory) {
    if (!users_.load_from_file(file_path(directory, "users.txt"), make_user)) {
        return false;
    }
    resources_.load_from_file(file_path(directory, "resources.txt"), make_resource);
    bookings_.load_from_file(file_path(directory, "bookings.txt"), make_booking);
    log_.entries().load_from_file(file_path(directory, "transactions.txt"), make_transaction);
    return true;
}

void GameCenter::seed_demo_data(Minutes now) {
    create_staff(Role::Owner, "owner", "owner123", "Владелец Центра");
    create_staff(Role::Administrator, "admin", "admin123", "Администратор Зала");

    const auto adult = register_client("client", "client123", "Иван Петров",
                                       timeutil::parse_date_days("1999-05-15"));
    const auto kid = register_client("kid", "kid12345", "Миша Сидоров", timeutil::parse_date_days("2014-03-10"));
    deposit(adult->id(), Money::from_double(300), now);
    deposit(kid->id(), Money::from_double(100), now);

    for (int i = 1; i <= 8; ++i) {
        add_resource(std::make_shared<PCZone>(resources_.next_id(), "PC-0" + std::to_string(i),
                                              Money::from_double(6), PcTier::Standard, "Intel Core i5-12400F",
                                              "NVIDIA RTX 3060", 16));
    }
    for (int i = 1; i <= 4; ++i) {
        add_resource(std::make_shared<PCZone>(resources_.next_id(), "VIP-0" + std::to_string(i),
                                              Money::from_double(6), PcTier::Vip, "Intel Core i7-14700K",
                                              "NVIDIA RTX 4080", 32));
    }
    add_resource(std::make_shared<LaserTagArena>(resources_.next_id(), "Лазертаг-арена", Money::from_double(12),
                                                 7, 60, 20, 10));
    add_resource(std::make_shared<RopePark>(resources_.next_id(), "Верёвочный городок", Money::from_double(10),
                                            5, 65, 15, 6));
    add_resource(std::make_shared<ClimbingWall>(resources_.next_id(), "Скалодром", Money::from_double(9), 8, 65,
                                                10, 8));
    add_resource(std::make_shared<RelaxZone>(resources_.next_id(), "Зона отдыха (кафе)", Money::from_double(40),
                                             30));
}

}  // namespace gc
