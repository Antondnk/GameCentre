#pragma once

#include <string>

#include "GameCenter.h"

namespace gc {

// Полный текстовый отчёт (сводка, аналитика, ресурсы, брони, транзакции, склад).
std::string build_text_report(const GameCenter& center, Minutes now);

// Журнал транзакций в формате CSV (разделитель ';' - открывается в Excel).
std::string build_transactions_csv(const GameCenter& center);

}  // namespace gc
