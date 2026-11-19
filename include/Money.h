#pragma once

#include <ostream>
#include <string>

namespace gc {

// Денежная сумма в копейках (целое число, чтобы не накапливать ошибку double).
class Money {
public:
    Money() = default;
    explicit Money(long long cents) : cents_(cents) {}

    static Money from_double(double rubles);
    // Разбор строки вида "12.5" или "12,50"; бросает InvalidInputException.
    static Money parse(const std::string& text);

    long long cents() const { return cents_; }
    std::string to_string() const;

    Money operator+(const Money& other) const { return Money(cents_ + other.cents_); }
    Money operator-(const Money& other) const { return Money(cents_ - other.cents_); }
    Money operator*(double factor) const;
    Money& operator+=(const Money& other);
    Money& operator-=(const Money& other);

    bool operator==(const Money& other) const { return cents_ == other.cents_; }
    bool operator!=(const Money& other) const { return cents_ != other.cents_; }
    bool operator<(const Money& other) const { return cents_ < other.cents_; }
    bool operator>(const Money& other) const { return cents_ > other.cents_; }
    bool operator<=(const Money& other) const { return cents_ <= other.cents_; }
    bool operator>=(const Money& other) const { return cents_ >= other.cents_; }

    // Дружественная функция: вывод суммы в поток.
    friend std::ostream& operator<<(std::ostream& out, const Money& money);

private:
    long long cents_ = 0;
};

}  // namespace gc
