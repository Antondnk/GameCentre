#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Money.h"

namespace gc {

enum class Role { Client, Administrator, Owner };

std::string to_string(Role role);

// Абстрактный пользователь системы.
class User {
public:
    User(int id, std::string login, std::string salt, std::string password_hash, std::string full_name);
    virtual ~User() = default;

    int id() const { return id_; }
    const std::string& login() const { return login_; }
    const std::string& full_name() const { return full_name_; }

    virtual Role get_role() const = 0;
    virtual std::string serialize() const = 0;

    // Сравнивает хэш введённого пароля с сохранённым.
    bool check_password(const std::string& password) const;

    static std::string generate_salt();
    static std::string make_hash(const std::string& salt, const std::string& password);

protected:
    std::vector<std::string> base_fields(const std::string& tag) const;

private:
    int id_;
    std::string login_;
    std::string salt_;
    std::string password_hash_;
    std::string full_name_;
};

// Клиент: баланс, дата рождения, персональная скидка.
class Client : public User {
public:
    Client(int id, std::string login, std::string salt, std::string password_hash, std::string full_name,
           long long birth_days, Money balance, int discount_percent);

    Role get_role() const override { return Role::Client; }
    std::string serialize() const override;

    long long birth_days() const { return birth_days_; }
    // Полных лет на указанный день (дни от 1970-01-01).
    int age_on(long long days) const;

    const Money& balance() const { return balance_; }
    void add_funds(const Money& amount);
    // Бросает InsufficientFundsException, если balance < amount.
    void charge(const Money& amount);

    int discount_percent() const { return discount_percent_; }
    void set_discount_percent(int percent);

private:
    long long birth_days_;
    Money balance_;
    int discount_percent_;
};

class Administrator : public User {
public:
    using User::User;

    Role get_role() const override { return Role::Administrator; }
    std::string serialize() const override;
};

// Владелец обладает всеми правами администратора и дополнительными (отчёты, персонал).
class Owner : public Administrator {
public:
    using Administrator::Administrator;

    Role get_role() const override { return Role::Owner; }
    std::string serialize() const override;
};

std::shared_ptr<User> make_user(const std::vector<std::string>& fields);

}  // namespace gc
