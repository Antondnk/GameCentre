#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Booking.h"
#include "Money.h"
#include "Repository.h"
#include "Resource.h"
#include "TimeSlot.h"
#include "Timeline.h"
#include "Transaction.h"
#include "User.h"

namespace gc {

// Фасад бизнес-логики: пользователи, ресурсы, бронирования, финансы, хранение.
// Не зависит от интерфейса, поэтому одинаково подойдёт консоли и будущему Qt GUI.
class GameCenter {
public:
    // ----- Пользователи -----
    std::shared_ptr<User> authenticate(const std::string& login, const std::string& password) const;
    std::shared_ptr<User> find_user_by_login(const std::string& login) const;
    std::shared_ptr<Client> register_client(const std::string& login, const std::string& password,
                                            const std::string& full_name, long long birth_days);
    std::shared_ptr<User> create_staff(Role role, const std::string& login, const std::string& password,
                                       const std::string& full_name);
    std::shared_ptr<Client> get_client(int client_id) const;
    Role get_role(int user_id) const;

    // ----- Финансы -----
    void deposit(int client_id, const Money& amount, Minutes now);
    void withdraw(int client_id, const Money& amount, const std::string& reason, Minutes now);
    // Стоимость с учётом персональной скидки клиента.
    Money quote(const Client& client, const Resource& resource, const TimeSlot& slot, int guests) const;

    // ----- Ресурсы -----
    void add_resource(std::shared_ptr<Resource> resource);
    void remove_resource(int resource_id);
    ResourceStatus get_device_status(int resource_id, Minutes now) const;
    void set_device_status(int resource_id, ResourceStatus status);
    void set_zone_maintenance(int resource_id, bool enabled);
    // Освобождает ресурсы, у которых истёк оплаченный сеанс.
    void refresh_statuses(Minutes now);
    bool is_resource_free(int resource_id, const TimeSlot& slot, int ignore_booking_id = -1) const;

    // ----- Возрастные ограничения -----
    // Несовершеннолетним нельзя находиться в клубе в ночное время.
    static void check_age_validity(const Client& client, const TimeSlot& slot);
    // Возраст должен входить в [min_age; max_age] зоны.
    static void validate_zone_age(int age, const Resource& resource);

    // ----- Сеансы и бронирования -----
    Money start_session(int client_id, int resource_id, int minutes, int guests, Minutes now);
    std::shared_ptr<Booking> create_booking(int client_id, int resource_id, const TimeSlot& slot, int guests,
                                            Minutes now);
    // requester_id < 0 - действие персонала (без проверки владельца брони). Возвращает сумму возврата.
    Money cancel_booking(int booking_id, int requester_id, Minutes now);
    std::shared_ptr<EventBooking> create_event_booking(int client_id, const std::string& event_name,
                                                       const EventTimeline& timeline, int guests, int avg_age,
                                                       Minutes now);
    EventTimeline generate_event_timeline(const EventRequest& request) const;

    // ----- Хранение -----
    void save_all(const std::string& directory) const;
    bool load_all(const std::string& directory);
    void seed_demo_data(Minutes now);

    const Repository<User>& users() const { return users_; }
    const Repository<Resource>& resources() const { return resources_; }
    const Repository<Booking>& bookings() const { return bookings_; }
    const TransactionLog& transactions() const { return log_; }

private:
    void require_free_login(const std::string& login) const;
    static void require_future(const TimeSlot& slot, Minutes now);

    Repository<User> users_;
    Repository<Resource> resources_;
    Repository<Booking> bookings_;
    TransactionLog log_;
};

}  // namespace gc
