#include "User.h"

#include <random>
#include <sstream>

#include "Exceptions.h"
#include "Sha256.h"
#include "TimeUtils.h"
#include "Utils.h"

namespace gc {

std::string to_string(Role role) {
    switch (role) {
        case Role::Client:
            return "Клиент";
        case Role::Administrator:
            return "Администратор";
        case Role::Owner:
            return "Владелец";
    }
    return "Неизвестно";
}

// -------------------------------------------------------------------- User

User::User(int id, std::string login, std::string salt, std::string password_hash, std::string full_name)
    : id_(id),
      login_(util::sanitize_field(std::move(login))),
      salt_(std::move(salt)),
      password_hash_(std::move(password_hash)),
      full_name_(util::sanitize_field(std::move(full_name))) {
    if (login_.empty()) {
        throw InvalidInputException("Логин не может быть пустым");
    }
}

bool User::check_password(const std::string& password) const {
    return make_hash(salt_, password) == password_hash_;
}

std::string User::generate_salt() {
    std::random_device device;
    std::ostringstream out;
    out << std::hex << device() << device();
    return out.str();
}

std::string User::make_hash(const std::string& salt, const std::string& password) {
    return sha256_hex(salt + password);
}

std::vector<std::string> User::base_fields(const std::string& tag) const {
    return {tag, std::to_string(id_), login_, salt_, password_hash_, full_name_};
}

// ------------------------------------------------------------------ Client

Client::Client(int id, std::string login, std::string salt, std::string password_hash,
               std::string full_name, long long birth_days, Money balance, int discount_percent)
    : User(id, std::move(login), std::move(salt), std::move(password_hash), std::move(full_name)),
      birth_days_(birth_days),
      balance_(balance),
      discount_percent_(0) {
    set_discount_percent(discount_percent);
}

std::string Client::serialize() const {
    std::vector<std::string> fields = base_fields("C");
    fields.push_back(std::to_string(birth_days_));
    fields.push_back(std::to_string(balance_.cents()));
    fields.push_back(std::to_string(discount_percent_));
    return util::join_values(fields, '|');
}

int Client::age_on(long long days) const {
    int birth_year = 0;
    unsigned birth_month = 0;
    unsigned birth_day = 0;
    timeutil::civil_from_days(birth_days_, birth_year, birth_month, birth_day);

    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    timeutil::civil_from_days(days, year, month, day);

    int age = year - birth_year;
    if (month < birth_month || (month == birth_month && day < birth_day)) {
        --age;
    }
    return age;
}

void Client::add_funds(const Money& amount) {
    balance_ += amount;
}

void Client::charge(const Money& amount) {
    if (balance_ < amount) {
        throw InsufficientFundsException(amount, balance_);
    }
    balance_ -= amount;
}

void Client::set_discount_percent(int percent) {
    if (percent < 0 || percent > 50) {
        throw InvalidInputException("Скидка должна быть в диапазоне 0-50%");
    }
    discount_percent_ = percent;
}

// --------------------------------------------------------- Administrator/Owner

std::string Administrator::serialize() const {
    return util::join_values(base_fields("A"), '|');
}

std::string Owner::serialize() const {
    return util::join_values(base_fields("O"), '|');
}

std::shared_ptr<User> make_user(const std::vector<std::string>& fields) {
    constexpr std::size_t kBase = 6;
    if (fields.size() < kBase) {
        throw FileFormatException("Недостаточно полей в записи пользователя");
    }
    const int id = util::parse_int(fields[1], "id");
    const std::string& tag = fields[0];

    if (tag == "C") {
        if (fields.size() < kBase + 3) {
            throw FileFormatException("Недостаточно полей в записи клиента");
        }
        return std::make_shared<Client>(id, fields[2], fields[3], fields[4], fields[5],
                                        util::parse_long_long(fields[6], "birth"),
                                        Money(util::parse_long_long(fields[7], "balance")),
                                        util::parse_int(fields[8], "discount"));
    }
    if (tag == "A") {
        return std::make_shared<Administrator>(id, fields[2], fields[3], fields[4], fields[5]);
    }
    if (tag == "O") {
        return std::make_shared<Owner>(id, fields[2], fields[3], fields[4], fields[5]);
    }
    throw FileFormatException("Неизвестный тип пользователя: " + tag);
}

}  // namespace gc
