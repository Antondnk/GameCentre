#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Money.h"
#include "Repository.h"
#include "TimeUtils.h"

namespace gc {

enum class TransactionType { Deposit = 0, Withdrawal = 1, Refund = 2 };

std::string to_string(TransactionType type);

class Transaction {
public:
    Transaction(int id, int client_id, Minutes timestamp, TransactionType type, Money amount,
                std::string description);

    int id() const { return id_; }
    int client_id() const { return client_id_; }
    Minutes timestamp() const { return timestamp_; }
    TransactionType type() const { return type_; }
    const Money& amount() const { return amount_; }
    const std::string& description() const { return description_; }

    std::string serialize() const;

private:
    int id_;
    int client_id_;
    Minutes timestamp_;
    TransactionType type_;
    Money amount_;
    std::string description_;
};

std::shared_ptr<Transaction> make_transaction(const std::vector<std::string>& fields);

// Журнал финансовых операций (TransactionLog).
class TransactionLog {
public:
    void record(int client_id, Minutes timestamp, TransactionType type, const Money& amount,
                const std::string& description);
    // Выручка = списания - возвраты.
    Money total_revenue() const;
    const Repository<Transaction>& entries() const { return entries_; }
    Repository<Transaction>& entries() { return entries_; }

private:
    Repository<Transaction> entries_;
};

}  // namespace gc
