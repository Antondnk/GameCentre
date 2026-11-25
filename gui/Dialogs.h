#pragma once

#include <QDialog>

#include <memory>
#include <string>

#include "GameCenter.h"

class QComboBox;
class QDateEdit;
class QDateTimeEdit;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QSpinBox;

// Бронирование: ресурс, начало, длительность, участники.
class BookingDialog : public QDialog {
public:
    BookingDialog(const gc::GameCenter& center, int preselected_resource_id, QWidget* parent);

    int resource_id() const;
    gc::Minutes start() const;
    int minutes() const;
    int guests() const;

private:
    QComboBox* resource_ = nullptr;
    QDateTimeEdit* start_ = nullptr;
    QSpinBox* minutes_ = nullptr;
    QSpinBox* guests_ = nullptr;
};

// Добавление ресурса любого типа (поля меняются в зависимости от типа).
class AddResourceDialog : public QDialog {
public:
    explicit AddResourceDialog(QWidget* parent);

    // Создаёт ресурс с заданным id по введённым данным (бросает исключения ядра при ошибках).
    std::shared_ptr<gc::Resource> create(int id) const;

private:
    struct Row {
        QLabel* label = nullptr;
        QWidget* field = nullptr;
    };
    void add_row(Row& row, const QString& caption, QWidget* field);
    void update_visibility();

    QComboBox* type_ = nullptr;
    QLineEdit* name_ = nullptr;
    QDoubleSpinBox* price_ = nullptr;
    QSpinBox* min_age_ = nullptr;
    QSpinBox* max_age_ = nullptr;
    QSpinBox* capacity_ = nullptr;
    QSpinBox* extra_number_ = nullptr;
    QLineEdit* text_one_ = nullptr;
    QLineEdit* text_two_ = nullptr;
    QSpinBox* ram_ = nullptr;
    QComboBox* tier_ = nullptr;

    Row min_age_row_, max_age_row_, capacity_row_, extra_row_, text_one_row_, text_two_row_, ram_row_, tier_row_;
};

// Новый товар магазина.
class ProductDialog : public QDialog {
public:
    explicit ProductDialog(QWidget* parent);

    std::string name() const;
    std::string category() const;
    gc::Money price() const;
    int stock() const;

private:
    QLineEdit* name_ = nullptr;
    QComboBox* category_ = nullptr;
    QDoubleSpinBox* price_ = nullptr;
    QSpinBox* stock_ = nullptr;
};

// Регистрация клиента сотрудником.
class ClientDialog : public QDialog {
public:
    explicit ClientDialog(QWidget* parent);

    std::string login() const;
    std::string password() const;
    std::string full_name() const;
    long long birth_days() const;

private:
    QLineEdit* login_ = nullptr;
    QLineEdit* password_ = nullptr;
    QLineEdit* full_name_ = nullptr;
    QDateEdit* birth_ = nullptr;
};
