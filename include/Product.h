#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Money.h"
#include "TimeUtils.h"

namespace gc {

// Допустимые категории товаров магазина/бара.
const std::vector<std::string>& product_categories();

// Товар магазина: напитки, еда, аксессуары, услуги.
class Product {
public:
    Product(int id, std::string name, std::string category, Money price, int stock);

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& category() const { return category_; }
    const Money& price() const { return price_; }
    int stock() const { return stock_; }

    void set_price(Money price);
    void add_stock(int quantity);
    // Бросает OutOfStockException, если товара не хватает.
    void remove_stock(int quantity);

    std::string serialize() const;

private:
    int id_;
    std::string name_;
    std::string category_;
    Money price_;
    int stock_;
};

std::shared_ptr<Product> make_product(const std::vector<std::string>& fields);

// Покупка клиента (для аналитики продаж).
class Order {
public:
    Order(int id, int client_id, Minutes timestamp, std::string product_name, std::string category,
          int quantity, Money total);

    int id() const { return id_; }
    int client_id() const { return client_id_; }
    Minutes timestamp() const { return timestamp_; }
    const std::string& product_name() const { return product_name_; }
    const std::string& category() const { return category_; }
    int quantity() const { return quantity_; }
    const Money& total() const { return total_; }

    std::string serialize() const;

private:
    int id_;
    int client_id_;
    Minutes timestamp_;
    std::string product_name_;
    std::string category_;
    int quantity_;
    Money total_;
};

std::shared_ptr<Order> make_order(const std::vector<std::string>& fields);

}  // namespace gc
