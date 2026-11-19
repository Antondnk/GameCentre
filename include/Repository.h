#pragma once

#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Exceptions.h"
#include "Utils.h"

namespace gc {

// Шаблонный репозиторий-контейнер. Требования к T: методы int id() const
// и std::string serialize() const.
template <typename T>
class Repository {
public:
    using Ptr = std::shared_ptr<T>;
    using Container = std::map<int, Ptr>;
    using const_iterator = typename Container::const_iterator;
    using Factory = std::function<Ptr(const std::vector<std::string>&)>;

    // Добавление с проверкой дубликата по id.
    void add_item(Ptr item) {
        if (!item) {
            throw InvalidInputException("Попытка добавить пустой объект");
        }
        const int id = item->id();
        if (!items_.emplace(id, std::move(item)).second) {
            throw DuplicateItemException("Элемент с id " + std::to_string(id) + " уже существует");
        }
    }

    void remove_item(int id) {
        const auto it = items_.find(id);
        if (it == items_.end()) {
            throw ItemNotFoundException("Элемент с id " + std::to_string(id) + " не найден");
        }
        items_.erase(it);
    }

    // nullptr, если элемента нет.
    Ptr find(int id) const {
        const auto it = items_.find(id);
        return it == items_.end() ? nullptr : it->second;
    }

    // Бросает ItemNotFoundException, если элемента нет.
    Ptr get(int id) const {
        Ptr item = find(id);
        if (!item) {
            throw ItemNotFoundException("Элемент с id " + std::to_string(id) + " не найден");
        }
        return item;
    }

    bool contains(int id) const { return items_.count(id) > 0; }
    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    void clear() { items_.clear(); }

    int next_id() const { return items_.empty() ? 1 : items_.rbegin()->first + 1; }

    const_iterator begin() const { return items_.begin(); }
    const_iterator end() const { return items_.end(); }

    std::vector<Ptr> values() const {
        std::vector<Ptr> result;
        result.reserve(items_.size());
        for (const auto& entry : items_) {
            result.push_back(entry.second);
        }
        return result;
    }

    template <typename Predicate>
    std::vector<Ptr> find_all(Predicate predicate) const {
        std::vector<Ptr> result;
        for (auto it = items_.begin(); it != items_.end(); ++it) {
            if (predicate(*it->second)) {
                result.push_back(it->second);
            }
        }
        return result;
    }

    // Каждая запись - одна строка, поля разделены '|'.
    void save_to_file(const std::string& path) const {
        std::ofstream file(path, std::ios::trunc);
        if (!file) {
            throw FileFormatException("Не удалось открыть файл для записи: " + path);
        }
        for (const auto& entry : items_) {
            file << entry.second->serialize() << '\n';
        }
    }

    // false, если файла нет; FileFormatException при ошибке формата.
    bool load_from_file(const std::string& path, const Factory& factory) {
        std::ifstream file(path);
        if (!file) {
            return false;
        }
        items_.clear();
        std::string line;
        int line_number = 0;
        while (std::getline(file, line)) {
            ++line_number;
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) {
                continue;
            }
            try {
                add_item(factory(util::split(line, '|')));
            } catch (const DuplicateItemException&) {
                throw;
            } catch (const std::exception& error) {
                throw FileFormatException(path + ", строка " + std::to_string(line_number) + ": " +
                                          error.what());
            }
        }
        return true;
    }

private:
    Container items_;
};

}  // namespace gc
