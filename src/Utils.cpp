#include "Utils.h"

#include <algorithm>
#include <cctype>

#include "Exceptions.h"

namespace gc::util {

std::vector<std::string> split(const std::string& text, char separator) {
    std::vector<std::string> parts;
    std::size_t begin = 0;
    while (true) {
        const std::size_t pos = text.find(separator, begin);
        if (pos == std::string::npos) {
            parts.push_back(text.substr(begin));
            break;
        }
        parts.push_back(text.substr(begin, pos - begin));
        begin = pos + 1;
    }
    return parts;
}

std::string trim(const std::string& text) {
    const auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    auto first = std::find_if_not(text.begin(), text.end(), is_space);
    auto last = std::find_if_not(text.rbegin(), text.rend(), is_space).base();
    return first < last ? std::string(first, last) : std::string();
}

std::string sanitize_field(std::string text) {
    for (char& c : text) {
        if (c == '|' || c == ';' || c == ',' || c == ':' || c == '\n' || c == '\r') {
            c = ' ';
        }
    }
    return trim(text);
}

int parse_int(const std::string& text, const std::string& field_name) {
    try {
        std::size_t pos = 0;
        const int value = std::stoi(text, &pos);
        if (pos != text.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return value;
    } catch (const std::logic_error&) {
        throw InvalidInputException("Некорректное целое число в поле \"" + field_name + "\": " + text);
    }
}

long long parse_long_long(const std::string& text, const std::string& field_name) {
    try {
        std::size_t pos = 0;
        const long long value = std::stoll(text, &pos);
        if (pos != text.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return value;
    } catch (const std::logic_error&) {
        throw InvalidInputException("Некорректное число в поле \"" + field_name + "\": " + text);
    }
}

}  // namespace gc::util
