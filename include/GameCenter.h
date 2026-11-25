#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Booking.h"
#include "Money.h"
#include "Product.h"
#include "Repository.h"
#include "Resource.h"
#include "TimeSlot.h"
#include "Timeline.h"
#include "Transaction.h"
#include "User.h"

namespace gc {

// Уровень лояльности клиента: считается по сумме потраченных средств.
struct LoyaltyLevel {
    std::string name;
    int discount_percent = 0;
    Money spent;
    Money next_threshold;  // 0, если достигнут максимальный уровень
    std::string next_name;
};

// Фасад бизнес-логики: пользователи, ресурсы, магазин, бронирования, финансы, хранение.
// Не зависит от интерфейса, поэтому одинаково подойдёт консоли и Qt GUI.
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
    // category: session / booking / event / shop / other (для аналитики).
    void withdraw(int client_id, const Money& amount, const std::string& reason, Minutes now,
                  const std::string& category = "other");
    // Стоимость с учётом скидки клиента (персональной или по уровню лояльности).
    Money quote(const Client& client, const Resource& resource, const TimeSlot& slot, int guests) const;

    // ----- Лояльность -----
    Money total_spent(int client_id) const;
    LoyaltyLevel loyalty_level(int client_id) const;
    int effective_discount(const Client& client) const;

    // ----- Ресурсы -----
    void add_resource(std::shared_ptr<Resource> resource);
    void remove_resource(int resource_id);
    void set_resource_price(int resource_id, const Money& price);
    ResourceStatus get_device_status(int resource_id, Minutes now) const;
    void set_device_status(int resource_id, ResourceStatus status);
    void set_zone_maintenance(int resource_id, bool enabled);
    // Освобождает ресурсы, у которых истёк оплаченный сеанс.
    void refresh_statuses(Minutes now);
    bool is_resource_free(int resource_id, const TimeSlot& slot, int ignore_booking_id = -1) const;

    // ----- Магазин -----
    std::shared_ptr<Product> add_product(const std::string& name, const std::string& category,
                                         const Money& price, int stock);
    void restock_product(int product_id, int quantity);
    void remove_product(int product_id);
    // Возвращает сумму покупки (с учётом скидки). Бросает OutOfStockException.
    Money buy_product(int client_id, int product_id, int quantity, Minutes now);

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

    // ----- Журнал действий -----
    void log_activity(Minutes now, const std::string& text);
    const std::vector<std::string>& activity_log() const { return activity_; }

    // ----- Хранение -----
    void save_all(const std::string& directory) const;
    bool load_all(const std::string& directory);
    void seed_demo_data(Minutes now);

    const Repository<User>& users() const { return users_; }
    const Repository<Resource>& resources() const { return resources_; }
    const Repository<Booking>& bookings() const { return bookings_; }
    const Repository<Product>& products() const { return products_; }
    const Repository<Order>& orders() const { return orders_; }
    const TransactionLog& transactions() const { return log_; }

private:
    void require_free_login(const std::string& login) const;
    static void require_future(const TimeSlot& slot, Minutes now);
    void seed_extra_resources();
    void seed_default_products();

    Repository<User> users_;
    Repository<Resource> resources_;
    Repository<Booking> bookings_;
    Repository<Product> products_;
    Repository<Order> orders_;
    TransactionLog log_;
    std::vector<std::string> activity_;
};

}  // namespace gc
