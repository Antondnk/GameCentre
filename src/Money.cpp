#include "Money.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "Exceptions.h"
#include "Utils.h"

namespace gc {

Money Money::from_double(double rubles) {
    return Money(std::llround(rubles * 100.0));
}

Money Money::parse(const std::string& text) {
    std::string value = util::trim(text);
    std::replace(value.begin(), value.end(), ',', '.');
    if (value.empty()) {
        throw InvalidInputException("Пустая денежная сумма");
    }
    try {
        std::size_t pos = 0;
        const double number = std::stod(value, &pos);
        if (pos != value.size() || !std::isfinite(number)) {
            throw std::invalid_argument("bad number");
        }
        return from_double(number);
    } catch (const std::logic_error&) {
        throw InvalidInputException("Некорректная денежная сумма: " + text);
    }
}

std::string Money::to_string() const {
    std::ostringstream out;
    const long long absolute = cents_ < 0 ? -cents_ : cents_;
    if (cents_ < 0) {
        out << '-';
    }
    out << absolute / 100 << '.' << std::setfill('0') << std::setw(2) << absolute % 100 << " BYN";
    return out.str();
}

Money Money::operator*(double factor) const {
    return Money(std::llround(static_cast<double>(cents_) * factor));
}

Money& Money::operator+=(const Money& other) {
    cents_ += other.cents_;
    return *this;
}

Money& Money::operator-=(const Money& other) {
    cents_ -= other.cents_;
    return *this;
}

std::ostream& operator<<(std::ostream& out, const Money& money) {
    return out << money.to_string();
}

}  // namespace gc
