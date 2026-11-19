#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace gc::util {

// Разбиение строки по разделителю (пустые поля сохраняются).
std::vector<std::string> split(const std::string& text, char separator);
std::string trim(const std::string& text);
// Убирает служебные символы файлового формата из пользовательского текста.
std::string sanitize_field(std::string text);

// Строгий разбор чисел; бросает InvalidInputException.
int parse_int(const std::string& text, const std::string& field_name);
long long parse_long_long(const std::string& text, const std::string& field_name);

// Шаблон функции: объединение значений любого типа в строку.
template <typename T>
std::string join_values(const std::vector<T>& values, char separator) {
    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << separator;
        }
        out << values[i];
    }
    return out.str();
}

// Шаблон функции: обратная операция, разбор значений типа T.
template <typename T>
std::vector<T> split_values(const std::string& text, char separator) {
    std::vector<T> result;
    for (const std::string& part : split(text, separator)) {
        std::istringstream in(part);
        T value{};
        if (in >> value) {
            result.push_back(value);
        }
    }
    return result;
}

}  // namespace gc::util
