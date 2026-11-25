#include "Dialogs.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

#include "Exceptions.h"
#include "GuiUtils.h"
#include "Product.h"

namespace {

QDialogButtonBox* make_buttons(QDialog* dialog) {
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    return buttons;
}

}  // namespace

// ------------------------------------------------------------- BookingDialog

BookingDialog::BookingDialog(const gc::GameCenter& center, int preselected_resource_id, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Бронирование");
    setMinimumWidth(400);
    auto* form = new QFormLayout(this);

    resource_ = new QComboBox(this);
    int index = 0;
    for (const auto& resource : center.resources().values()) {
        resource_->addItem(qs(resource->name()) + "  (" + qs(resource->kind()) + ")", resource->id());
        if (resource->id() == preselected_resource_id) {
            resource_->setCurrentIndex(index);
        }
        ++index;
    }
    start_ = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(3600), this);
    start_->setCalendarPopup(true);
    start_->setDisplayFormat("yyyy-MM-dd HH:mm");
    minutes_ = new QSpinBox(this);
    minutes_->setRange(30, 600);
    minutes_->setSingleStep(30);
    minutes_->setValue(60);
    minutes_->setSuffix(" мин");
    guests_ = new QSpinBox(this);
    guests_->setRange(1, 100);

    form->addRow("Ресурс:", resource_);
    form->addRow("Начало:", start_);
    form->addRow("Длительность:", minutes_);
    form->addRow("Участников:", guests_);
    form->addRow(make_buttons(this));
}

int BookingDialog::resource_id() const {
    return resource_->currentData().toInt();
}

gc::Minutes BookingDialog::start() const {
    return to_minutes(start_->dateTime());
}

int BookingDialog::minutes() const {
    return minutes_->value();
}

int BookingDialog::guests() const {
    return guests_->value();
}

// --------------------------------------------------------- AddResourceDialog

void AddResourceDialog::add_row(Row& row, const QString& caption, QWidget* field) {
    auto* grid = static_cast<QGridLayout*>(layout());
    const int index = grid->rowCount();
    row.label = new QLabel(caption, this);
    row.field = field;
    grid->addWidget(row.label, index, 0);
    grid->addWidget(field, index, 1);
}

AddResourceDialog::AddResourceDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Добавить ресурс");
    setMinimumWidth(420);
    auto* grid = new QGridLayout(this);

    type_ = new QComboBox(this);
    const QStringList types = {"Компьютер (ПК)", "Лазертаг",       "Верёвочный городок", "Скалодром",
                               "Зона отдыха",     "VR-арена",       "Игровые консоли",    "Бильярдный стол"};
    for (const QString& type : types) {
        type_->addItem(type);
    }
    name_ = new QLineEdit(this);
    price_ = new QDoubleSpinBox(this);
    price_->setRange(0.0, 10000.0);
    price_->setDecimals(2);
    price_->setValue(10.0);
    price_->setSuffix(" BYN");
    min_age_ = new QSpinBox(this);
    min_age_->setRange(0, 120);
    min_age_->setValue(6);
    max_age_ = new QSpinBox(this);
    max_age_->setRange(0, 120);
    max_age_->setValue(65);
    capacity_ = new QSpinBox(this);
    capacity_->setRange(1, 200);
    capacity_->setValue(10);
    extra_number_ = new QSpinBox(this);
    extra_number_->setRange(1, 100);
    extra_number_->setValue(4);
    text_one_ = new QLineEdit(this);
    text_two_ = new QLineEdit(this);
    ram_ = new QSpinBox(this);
    ram_->setRange(4, 256);
    ram_->setValue(16);
    tier_ = new QComboBox(this);
    tier_->addItem("Стандарт");
    tier_->addItem("VIP");

    grid->addWidget(new QLabel("Тип:", this), 0, 0);
    grid->addWidget(type_, 0, 1);
    grid->addWidget(new QLabel("Название:", this), 1, 0);
    grid->addWidget(name_, 1, 1);
    grid->addWidget(new QLabel("Цена (за час / за человека в час):", this), 2, 0);
    grid->addWidget(price_, 2, 1);

    add_row(min_age_row_, "Мин. возраст:", min_age_);
    add_row(max_age_row_, "Макс. возраст:", max_age_);
    add_row(capacity_row_, "Вместимость:", capacity_);
    add_row(extra_row_, "Дополнительно:", extra_number_);
    add_row(text_one_row_, "CPU / модель:", text_one_);
    add_row(text_two_row_, "GPU:", text_two_);
    add_row(ram_row_, "ОЗУ, ГБ:", ram_);
    add_row(tier_row_, "Тариф:", tier_);
    grid->addWidget(make_buttons(this), grid->rowCount(), 0, 1, 2);

    connect(type_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { update_visibility(); });
    update_visibility();
}

void AddResourceDialog::update_visibility() {
    const int type = type_->currentIndex();
    const bool is_pc = type == 0;
    const bool has_ages = type == 1 || type == 2 || type == 3 || type == 5;
    const bool has_extra = has_ages;
    const bool has_text_one = is_pc || type == 6;

    const auto show = [](const Row& row, bool visible) {
        row.label->setVisible(visible);
        row.field->setVisible(visible);
    };
    show(min_age_row_, has_ages);
    show(max_age_row_, has_ages);
    show(capacity_row_, !is_pc);
    show(extra_row_, has_extra);
    show(text_one_row_, has_text_one);
    show(text_two_row_, is_pc);
    show(ram_row_, is_pc);
    show(tier_row_, is_pc);

    const QStringList extra_captions = {"", "Игроков в команде:", "Макс. высота, м:", "Высота стенки, м:", "", "Количество шлемов:"};
    if (type >= 0 && type < extra_captions.size()) {
        extra_row_.label->setText(extra_captions.at(type));
    }
    text_one_row_.label->setText(is_pc ? "Процессор (CPU):" : "Модель консоли:");
}

std::shared_ptr<gc::Resource> AddResourceDialog::create(int id) const {
    const std::string name = name_->text().toStdString();
    if (name.empty()) {
        throw gc::InvalidInputException("Введите название ресурса");
    }
    const gc::Money price = gc::Money::from_double(price_->value());
    const int min_age = min_age_->value();
    const int max_age = max_age_->value();
    const int capacity = capacity_->value();
    const int extra = extra_number_->value();

    switch (type_->currentIndex()) {
        case 0:
            return std::make_shared<gc::PCZone>(id, name, price, tier_->currentIndex() == 1 ? gc::PcTier::Vip : gc::PcTier::Standard,
                                                text_one_->text().toStdString(), text_two_->text().toStdString(),
                                                ram_->value());
        case 1:
            return std::make_shared<gc::LaserTagArena>(id, name, price, min_age, max_age, capacity, extra);
        case 2:
            return std::make_shared<gc::RopePark>(id, name, price, min_age, max_age, capacity, extra);
        case 3:
            return std::make_shared<gc::ClimbingWall>(id, name, price, min_age, max_age, capacity, extra);
        case 4:
            return std::make_shared<gc::RelaxZone>(id, name, price, capacity);
        case 5:
            return std::make_shared<gc::VRZone>(id, name, price, min_age, max_age, capacity, extra);
        case 6:
            return std::make_shared<gc::ConsoleZone>(id, name, price, capacity, text_one_->text().toStdString());
        default:
            return std::make_shared<gc::BilliardsTable>(id, name, price, capacity);
    }
}

// ------------------------------------------------------------- ProductDialog

ProductDialog::ProductDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Новый товар");
    setMinimumWidth(360);
    auto* form = new QFormLayout(this);
    name_ = new QLineEdit(this);
    category_ = new QComboBox(this);
    for (const std::string& category : gc::product_categories()) {
        category_->addItem(qs(category));
    }
    price_ = new QDoubleSpinBox(this);
    price_->setRange(0.0, 10000.0);
    price_->setDecimals(2);
    price_->setValue(3.0);
    price_->setSuffix(" BYN");
    stock_ = new QSpinBox(this);
    stock_->setRange(0, 100000);
    stock_->setValue(20);
    form->addRow("Название:", name_);
    form->addRow("Категория:", category_);
    form->addRow("Цена:", price_);
    form->addRow("Остаток на складе:", stock_);
    form->addRow(make_buttons(this));
}

std::string ProductDialog::name() const {
    return name_->text().toStdString();
}

std::string ProductDialog::category() const {
    return category_->currentText().toStdString();
}

gc::Money ProductDialog::price() const {
    return gc::Money::from_double(price_->value());
}

int ProductDialog::stock() const {
    return stock_->value();
}

// -------------------------------------------------------------- ClientDialog

ClientDialog::ClientDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Новый клиент");
    setMinimumWidth(380);
    auto* form = new QFormLayout(this);
    login_ = new QLineEdit(this);
    password_ = new QLineEdit(this);
    password_->setEchoMode(QLineEdit::Password);
    full_name_ = new QLineEdit(this);
    birth_ = new QDateEdit(QDate(2000, 1, 1), this);
    birth_->setCalendarPopup(true);
    birth_->setDisplayFormat("yyyy-MM-dd");
    birth_->setMaximumDate(QDate::currentDate());
    form->addRow("Логин:", login_);
    form->addRow("Пароль (мин. 4 символа):", password_);
    form->addRow("ФИО:", full_name_);
    form->addRow("Дата рождения:", birth_);
    form->addRow(make_buttons(this));
}

std::string ClientDialog::login() const {
    return login_->text().toStdString();
}

std::string ClientDialog::password() const {
    return password_->text().toStdString();
}

std::string ClientDialog::full_name() const {
    return full_name_->text().toStdString();
}

long long ClientDialog::birth_days() const {
    return to_day_index(birth_->date());
}
