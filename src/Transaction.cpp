#include "Transaction.h"

#include "Exceptions.h"
#include "Utils.h"

namespace gc {

std::string to_string(TransactionType type) {
    switch (type) {
        case TransactionType::Deposit:
            return "Пополнение";
        case TransactionType::Withdrawal:
            return "Списание";
        case TransactionType::Refund:
            return "Возврат";
    }
    return "Неизвестно";
}

Transaction::Transaction(int id, int client_id, Minutes timestamp, TransactionType type, Money amount,
                         std::string description, std::string category)
    : id_(id),
      client_id_(client_id),
      timestamp_(timestamp),
      type_(type),
      amount_(amount),
      description_(util::sanitize_field(std::move(description))),
      category_(util::sanitize_field(std::move(category))) {}

std::string Transaction::serialize() const {
    const std::vector<std::string> fields = {std::to_string(id_),
                                             std::to_string(client_id_),
                                             std::to_string(timestamp_),
                                             std::to_string(static_cast<int>(type_)),
                                             std::to_string(amount_.cents()),
                                             description_,
                                             category_};
    return util::join_values(fields, '|');
}

std::shared_ptr<Transaction> make_transaction(const std::vector<std::string>& fields) {
    if (fields.size() < 6) {
        throw FileFormatException("Недостаточно полей в записи транзакции");
    }
    return std::make_shared<Transaction>(
        util::parse_int(fields[0], "id"), util::parse_int(fields[1], "client_id"),
        util::parse_long_long(fields[2], "timestamp"),
        static_cast<TransactionType>(util::parse_int(fields[3], "type")),
        Money(util::parse_long_long(fields[4], "amount")), fields[5],
        fields.size() > 6 ? fields[6] : std::string("other"));
}

void TransactionLog::record(int client_id, Minutes timestamp, TransactionType type, const Money& amount,
                            const std::string& description, const std::string& category) {
    entries_.add_item(std::make_shared<Transaction>(entries_.next_id(), client_id, timestamp, type, amount,
                                                    description, category));
}

Money TransactionLog::total_revenue() const {
    Money total;
    for (const auto& [id, entry] : entries_) {
        if (entry->type() == TransactionType::Withdrawal) {
            total += entry->amount();
        } else if (entry->type() == TransactionType::Refund) {
            total -= entry->amount();
        }
    }
    return total;
}

}  // namespace gc
