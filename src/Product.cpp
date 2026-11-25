#include "Product.h"

#include "Exceptions.h"
#include "Utils.h"

namespace gc {

const std::vector<std::string>& product_categories() {
    static const std::vector<std::string> categories = {"Напитки", "Еда", "Аксессуары", "Услуги"};
    return categories;
}

Product::Product(int id, std::string name, std::string category, Money price, int stock)
    : id_(id),
      name_(util::sanitize_field(std::move(name))),
      category_(util::sanitize_field(std::move(category))),
      price_(price),
      stock_(stock) {
    if (name_.empty()) {
        throw InvalidInputException("Название товара не может быть пустым");
    }
    if (price.cents() < 0 || stock < 0) {
        throw InvalidInputException("Цена и остаток не могут быть отрицательными");
    }
}

void Product::set_price(Money price) {
    if (price.cents() < 0) {
        throw InvalidInputException("Цена не может быть отрицательной");
    }
    price_ = price;
}

void Product::add_stock(int quantity) {
    if (quantity < 1) {
        throw InvalidInputException("Количество должно быть положительным");
    }
    stock_ += quantity;
}

void Product::remove_stock(int quantity) {
    if (quantity < 1) {
        throw InvalidInputException("Количество должно быть положительным");
    }
    if (quantity > stock_) {
        throw OutOfStockException("Товара \"" + name_ + "\" недостаточно: осталось " + std::to_string(stock_));
    }
    stock_ -= quantity;
}

std::string Product::serialize() const {
    const std::vector<std::string> fields = {std::to_string(id_), name_, category_,
                                             std::to_string(price_.cents()), std::to_string(stock_)};
    return util::join_values(fields, '|');
}

std::shared_ptr<Product> make_product(const std::vector<std::string>& fields) {
    if (fields.size() < 5) {
        throw FileFormatException("Недостаточно полей в записи товара");
    }
    return std::make_shared<Product>(util::parse_int(fields[0], "id"), fields[1], fields[2],
                                     Money(util::parse_long_long(fields[3], "price")),
                                     util::parse_int(fields[4], "stock"));
}

Order::Order(int id, int client_id, Minutes timestamp, std::string product_name, std::string category,
             int quantity, Money total)
    : id_(id),
      client_id_(client_id),
      timestamp_(timestamp),
      product_name_(util::sanitize_field(std::move(product_name))),
      category_(util::sanitize_field(std::move(category))),
      quantity_(quantity),
      total_(total) {}

std::string Order::serialize() const {
    const std::vector<std::string> fields = {std::to_string(id_),
                                             std::to_string(client_id_),
                                             std::to_string(timestamp_),
                                             product_name_,
                                             category_,
                                             std::to_string(quantity_),
                                             std::to_string(total_.cents())};
    return util::join_values(fields, '|');
}

std::shared_ptr<Order> make_order(const std::vector<std::string>& fields) {
    if (fields.size() < 7) {
        throw FileFormatException("Недостаточно полей в записи заказа");
    }
    return std::make_shared<Order>(util::parse_int(fields[0], "id"), util::parse_int(fields[1], "client_id"),
                                   util::parse_long_long(fields[2], "timestamp"), fields[3], fields[4],
                                   util::parse_int(fields[5], "quantity"),
                                   Money(util::parse_long_long(fields[6], "total")));
}

}  // namespace gc
