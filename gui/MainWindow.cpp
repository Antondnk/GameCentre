#include "MainWindow.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

#include "Exceptions.h"
#include "TimeUtils.h"
#include "TimelineView.h"

namespace {

QString qs(const std::string& text) {
    return QString::fromUtf8(text.c_str());
}

QString money_text(const gc::Money& money) {
    return qs(money.to_string());
}

// QDateTime -> минуты ядра (локальное время без часовых поясов).
gc::Minutes to_minutes(const QDateTime& value) {
    const QDate date = value.date();
    const QTime time = value.time();
    return gc::timeutil::days_from_civil(date.year(), static_cast<unsigned>(date.month()),
                                         static_cast<unsigned>(date.day())) *
               gc::kMinutesPerDay +
           time.hour() * 60 + time.minute();
}

long long to_day_index(const QDate& date) {
    return gc::timeutil::days_from_civil(date.year(), static_cast<unsigned>(date.month()),
                                         static_cast<unsigned>(date.day()));
}

void set_cell(QTableWidget* table, int row, int column, const QString& text) {
    table->setItem(row, column, new QTableWidgetItem(text));
}

QTableWidget* make_table(QWidget* parent, const QStringList& headers) {
    auto* table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    return table;
}

QLabel* make_section_title(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName("sectionTitle");
    return label;
}

// Цвет рамки кнопки ресурса зависит от статуса.
QString status_color(gc::ResourceStatus status) {
    switch (status) {
        case gc::ResourceStatus::Free:
            return "#2ecc71";
        case gc::ResourceStatus::Occupied:
            return "#ef5350";
        case gc::ResourceStatus::Booked:
            return "#fbc02d";
        case gc::ResourceStatus::Maintenance:
            return "#7b8094";
    }
    return "#7b8094";
}

QString zone_button_style(gc::ResourceStatus status) {
    const QString color = status_color(status);
    return "QPushButton { background-color: #1a1d29; border: 2px solid " + color +
           "; border-radius: 12px; color: #e6e8f0; font-weight: bold; }"
           "QPushButton:hover { background-color: #262b40; }";
}

// Диалог параметров брони: ресурс, начало, длительность, число участников.
class BookingDialog : public QDialog {
public:
    BookingDialog(const gc::GameCenter& center, int preselected_resource_id, QWidget* parent) : QDialog(parent) {
        setWindowTitle("Бронирование");
        setMinimumWidth(380);
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

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        form->addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    int resource_id() const { return resource_->currentData().toInt(); }
    gc::Minutes start() const { return to_minutes(start_->dateTime()); }
    int minutes() const { return minutes_->value(); }
    int guests() const { return guests_->value(); }

private:
    QComboBox* resource_ = nullptr;
    QDateTimeEdit* start_ = nullptr;
    QSpinBox* minutes_ = nullptr;
    QSpinBox* guests_ = nullptr;
};

}  // namespace

// ---------------------------------------------------------------- каркас окна

MainWindow::MainWindow(gc::GameCenter& center, std::shared_ptr<gc::User> user, std::string data_dir,
                       QWidget* parent)
    : QMainWindow(parent), center_(center), user_(std::move(user)), data_dir_(std::move(data_dir)) {
    setWindowTitle("Информационная система игрового центра");
    resize(1280, 780);

    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    sidebar_ = new QFrame(central);
    sidebar_->setObjectName("sidebar");
    sidebar_->setFixedWidth(230);
    nav_layout_ = new QVBoxLayout(sidebar_);
    nav_layout_->setContentsMargins(12, 8, 12, 12);
    nav_layout_->setSpacing(4);
    auto* logo = new QLabel("GAME CENTER", sidebar_);
    logo->setObjectName("logo");
    nav_layout_->addWidget(logo);

    auto* content = new QWidget(central);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(24, 16, 24, 16);
    content_layout->setSpacing(12);
    build_header(content_layout);
    stack_ = new QStackedWidget(content);
    content_layout->addWidget(stack_, 1);

    root->addWidget(sidebar_);
    root->addWidget(content, 1);
    setCentralWidget(central);

    build_dashboard_page();
    build_map_page();
    build_timeline_page();
    if (is_client()) {
        build_event_page();
    } else {
        build_clients_page();
    }
    if (is_owner()) {
        build_finance_page();
    }
    nav_layout_->addStretch();

    // Обновление карты зала по таймеру (освобождение ресурсов по окончании сеанса).
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this] {
        refresh_hall_map();
        refresh_header();
        refresh_dashboard();
    });
    timer_->start(5000);

    show_page(0);
}

void MainWindow::build_header(QVBoxLayout* content_layout) {
    auto* header = new QHBoxLayout();
    page_title_ = new QLabel(this);
    page_title_->setObjectName("pageTitle");
    user_label_ = new QLabel(this);
    user_label_->setObjectName("chip");
    balance_label_ = new QLabel(this);
    balance_label_->setObjectName("chip");

    header->addWidget(page_title_);
    header->addStretch();
    header->addWidget(user_label_);
    if (is_client()) {
        header->addWidget(balance_label_);
        auto* deposit = new QPushButton("Пополнить", this);
        deposit->setObjectName("accent");
        connect(deposit, &QPushButton::clicked, this, [this] { on_deposit_clicked(); });
        header->addWidget(deposit);
    } else {
        balance_label_->hide();
    }
    auto* logout = new QPushButton("Выйти", this);
    connect(logout, &QPushButton::clicked, this, [this] {
        logout_requested_ = true;
        close();
    });
    header->addWidget(logout);
    content_layout->addLayout(header);
}

void MainWindow::add_page(const QString& title, QWidget* page) {
    const int index = stack_->addWidget(page);
    auto* button = new QPushButton(title, sidebar_);
    button->setObjectName("navButton");
    button->setCheckable(true);
    button->setAutoExclusive(true);
    nav_layout_->addWidget(button);
    nav_buttons_.push_back(button);
    page_titles_.push_back(title);
    connect(button, &QPushButton::clicked, this, [this, index] { show_page(index); });
}

void MainWindow::show_page(int index) {
    stack_->setCurrentIndex(index);
    nav_buttons_[static_cast<std::size_t>(index)]->setChecked(true);
    page_title_->setText(page_titles_[static_cast<std::size_t>(index)]);
    refresh_all();
}

// ------------------------------------------------------------------ страницы

void MainWindow::build_dashboard_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    struct CardDef {
        const char* key;
        const char* caption;
        const char* color;
    };
    std::vector<CardDef> cards = {{"free", "Свободно", "#2ecc71"},
                                  {"occupied", "Занято", "#ef5350"},
                                  {"booked", "Забронировано", "#fbc02d"},
                                  {"maintenance", "На обслуживании", "#9aa0b5"},
                                  {"today", "Броней на сегодня", "#4c6fff"}};
    if (is_client()) {
        cards.push_back({"mine", "Мои брони", "#e6e8f0"});
    } else {
        cards.push_back({"clients", "Клиентов", "#e6e8f0"});
    }
    if (is_owner()) {
        cards.push_back({"revenue", "Выручка", "#2ecc71"});
    }

    auto* grid = new QGridLayout();
    grid->setSpacing(12);
    int index = 0;
    for (const CardDef& def : cards) {
        auto* card = new QFrame(page);
        card->setObjectName("card");
        auto* card_layout = new QVBoxLayout(card);
        auto* caption = new QLabel(QString::fromUtf8(def.caption), card);
        caption->setObjectName("cardCaption");
        auto* value = new QLabel("0", card);
        value->setObjectName("cardValue");
        value->setStyleSheet(QString("color: ") + QString::fromUtf8(def.color) + ";");
        card_layout->addWidget(caption);
        card_layout->addWidget(value);
        grid->addWidget(card, index / 4, index % 4);
        stat_values_[def.key] = value;
        ++index;
    }
    layout->addLayout(grid);

    layout->addWidget(make_section_title("Ближайшие бронирования", page));
    upcoming_table_ = make_table(page, {"Начало", "Тип", "Ресурсы", "Клиент"});
    layout->addWidget(upcoming_table_, 1);
    add_page("Рабочий стол", page);
}

void MainWindow::build_map_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* legend = new QLabel(
        "<span style='color:#2ecc71'>●</span> свободен &nbsp;&nbsp; "
        "<span style='color:#ef5350'>●</span> занят &nbsp;&nbsp; "
        "<span style='color:#fbc02d'>●</span> забронирован &nbsp;&nbsp; "
        "<span style='color:#7b8094'>●</span> на обслуживании &nbsp;&nbsp;—&nbsp;&nbsp; "
        "нажмите на зону, чтобы увидеть характеристики и действия",
        page);
    legend->setObjectName("muted");
    layout->addWidget(legend);

    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    auto* inner = new QWidget(scroll);
    auto* inner_layout = new QVBoxLayout(inner);

    std::map<std::string, std::pair<QGridLayout*, int>> grids;
    constexpr int kColumns = 5;
    for (const auto& resource : center_.resources().values()) {
        auto found = grids.find(resource->kind());
        if (found == grids.end()) {
            auto* box = new QGroupBox(qs(resource->kind()), inner);
            auto* grid = new QGridLayout(box);
            grid->setSpacing(10);
            inner_layout->addWidget(box);
            found = grids.emplace(resource->kind(), std::make_pair(grid, 0)).first;
        }
        auto* button = new QPushButton(inner);
        button->setMinimumSize(150, 70);
        const int index = found->second.second++;
        found->second.first->addWidget(button, index / kColumns, index % kColumns);

        const int resource_id = resource->id();
        connect(button, &QPushButton::clicked, this, [this, resource_id] { on_zone_clicked(resource_id); });
        zone_buttons_[resource_id] = button;
    }
    inner_layout->addStretch();
    scroll->setWidget(inner);
    layout->addWidget(scroll, 1);
    add_page("Карта зала", page);
}

void MainWindow::build_timeline_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* top = new QHBoxLayout();
    auto* previous = new QPushButton("←", page);
    auto* next = new QPushButton("→", page);
    auto* today = new QPushButton("Сегодня", page);
    timeline_date_ = new QDateEdit(QDate::currentDate(), page);
    timeline_date_->setCalendarPopup(true);
    timeline_date_->setDisplayFormat("dd.MM.yyyy");
    top->addWidget(previous);
    top->addWidget(timeline_date_);
    top->addWidget(next);
    top->addWidget(today);
    top->addStretch();
    if (is_client()) {
        auto* book = new QPushButton("+ Забронировать", page);
        book->setObjectName("accent");
        connect(book, &QPushButton::clicked, this, [this] { on_book_clicked(-1); });
        top->addWidget(book);
    }
    layout->addLayout(top);

    timeline_ = new TimelineView(center_, page);
    timeline_->set_viewer(user_->id(), is_client());
    timeline_->set_day(to_day_index(QDate::currentDate()));
    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setWidget(timeline_);
    layout->addWidget(scroll, 3);

    auto* legend = new QLabel(
        "<span style='color:#3b5bdb'>■</span> бронь &nbsp;&nbsp; "
        "<span style='color:#7c4dff'>■</span> мероприятие &nbsp;&nbsp; "
        "<span style='color:#e53935'>■</span> идёт сеанс &nbsp;&nbsp; "
        "<span style='color:#6e748c'>■</span> обслуживание &nbsp;&nbsp; "
        "<span style='color:#ef5350'>|</span> текущее время",
        page);
    legend->setObjectName("muted");
    layout->addWidget(legend);

    bookings_table_ = make_table(page, {"№", "Тип", "Клиент", "Ресурсы", "Время", "Стоимость"});
    layout->addWidget(bookings_table_, 2);
    auto* cancel = new QPushButton("Отменить выбранную бронь", page);
    connect(cancel, &QPushButton::clicked, this, [this] { on_cancel_booking_clicked(); });
    layout->addWidget(cancel, 0, Qt::AlignRight);

    connect(timeline_date_, &QDateEdit::dateChanged, this,
            [this](const QDate& date) { timeline_->set_day(to_day_index(date)); });
    connect(previous, &QPushButton::clicked, this,
            [this] { timeline_date_->setDate(timeline_date_->date().addDays(-1)); });
    connect(next, &QPushButton::clicked, this,
            [this] { timeline_date_->setDate(timeline_date_->date().addDays(1)); });
    connect(today, &QPushButton::clicked, this, [this] { timeline_date_->setDate(QDate::currentDate()); });

    add_page(is_client() ? "Бронирование" : "Все бронирования", page);
}

void MainWindow::build_event_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* info = new QLabel(
        "Конструктор подбирает последовательность активностей (например, лазертаг → ПК-турнир → отдых) "
        "с учётом числа гостей, возраста, свободных зон и бюджета.",
        page);
    info->setObjectName("muted");
    info->setWordWrap(true);
    layout->addWidget(info);

    auto* form = new QFormLayout();
    guests_spin_ = new QSpinBox(page);
    guests_spin_->setRange(1, 100);
    guests_spin_->setValue(8);
    age_spin_ = new QSpinBox(page);
    age_spin_->setRange(1, 99);
    age_spin_->setValue(12);
    budget_spin_ = new QDoubleSpinBox(page);
    budget_spin_->setRange(1.0, 100000.0);
    budget_spin_->setDecimals(2);
    budget_spin_->setValue(300.0);
    budget_spin_->setSuffix(" BYN");
    start_edit_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), page);
    start_edit_->setTime(QTime(14, 0));
    start_edit_->setCalendarPopup(true);
    start_edit_->setDisplayFormat("yyyy-MM-dd HH:mm");
    stages_spin_ = new QSpinBox(page);
    stages_spin_->setRange(1, 5);
    stages_spin_->setValue(3);

    form->addRow("Количество гостей:", guests_spin_);
    form->addRow("Средний возраст:", age_spin_);
    form->addRow("Бюджет:", budget_spin_);
    form->addRow("Начало праздника:", start_edit_);
    form->addRow("Количество этапов (по часу):", stages_spin_);
    layout->addLayout(form);

    auto* generate = new QPushButton("Подобрать программу", page);
    generate->setObjectName("accent");
    connect(generate, &QPushButton::clicked, this, [this] { on_generate_event_clicked(); });
    layout->addWidget(generate);

    timeline_table_ = make_table(page, {"Время", "Активность", "Ресурсы", "Стоимость"});
    layout->addWidget(timeline_table_, 1);
    timeline_summary_ = new QLabel(page);
    timeline_summary_->setObjectName("sectionTitle");
    layout->addWidget(timeline_summary_);

    book_event_button_ = new QPushButton("Забронировать мероприятие", page);
    book_event_button_->setEnabled(false);
    connect(book_event_button_, &QPushButton::clicked, this, [this] { on_book_event_clicked(); });
    layout->addWidget(book_event_button_);
    add_page("Конструктор праздника", page);
}

void MainWindow::build_clients_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    clients_table_ = make_table(page, {"ID", "Логин", "ФИО", "Возраст", "Баланс", "Скидка"});
    layout->addWidget(clients_table_, 1);

    auto* buttons = new QHBoxLayout();
    auto* deposit = new QPushButton("Пополнить счёт", page);
    deposit->setObjectName("accent");
    auto* discount = new QPushButton("Задать скидку", page);
    connect(deposit, &QPushButton::clicked, this, [this] { on_client_deposit_clicked(); });
    connect(discount, &QPushButton::clicked, this, [this] { on_client_discount_clicked(); });
    buttons->addStretch();
    buttons->addWidget(deposit);
    buttons->addWidget(discount);
    layout->addLayout(buttons);
    add_page("Клиенты", page);
}

void MainWindow::build_finance_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    transactions_table_ = make_table(page, {"№", "Время", "Клиент", "Тип", "Сумма", "Описание"});
    layout->addWidget(transactions_table_, 1);
    revenue_label_ = new QLabel(page);
    revenue_label_->setObjectName("sectionTitle");
    layout->addWidget(revenue_label_);
    add_page("Финансы", page);
}

// ------------------------------------------------------------------ обновление

void MainWindow::refresh_all() {
    refresh_header();
    refresh_dashboard();
    refresh_hall_map();
    refresh_bookings();
    timeline_->reload();
    if (clients_table_) {
        refresh_clients();
    }
    if (transactions_table_) {
        refresh_transactions();
    }
}

void MainWindow::refresh_header() {
    user_label_->setText(qs(user_->full_name()) + "  ·  " + qs(gc::to_string(user_->get_role())));
    if (is_client()) {
        balance_label_->setText("Баланс: " + money_text(center_.get_client(user_->id())->balance()));
    }
}

void MainWindow::set_stat(const std::string& key, const QString& text) {
    const auto found = stat_values_.find(key);
    if (found != stat_values_.end()) {
        found->second->setText(text);
    }
}

std::vector<std::shared_ptr<gc::Booking>> MainWindow::visible_bookings() const {
    std::vector<std::shared_ptr<gc::Booking>> shown;
    for (const auto& booking : center_.bookings().values()) {
        if (!is_client() || booking->client_id() == user_->id()) {
            shown.push_back(booking);
        }
    }
    return shown;
}

QString MainWindow::booking_resource_names(const gc::Booking& booking) const {
    QStringList names;
    for (const int id : booking.resource_ids()) {
        const auto resource = center_.resources().find(id);
        names << (resource ? qs(resource->name()) : QString("(удалён)"));
    }
    return names.join(", ");
}

void MainWindow::refresh_dashboard() {
    const gc::Minutes now = gc::timeutil::now_minutes();
    int counts[4] = {0, 0, 0, 0};
    for (const auto& resource : center_.resources().values()) {
        const gc::ResourceStatus status = center_.get_device_status(resource->id(), now);
        ++counts[static_cast<int>(status)];
    }
    set_stat("free", QString::number(counts[static_cast<int>(gc::ResourceStatus::Free)]));
    set_stat("occupied", QString::number(counts[static_cast<int>(gc::ResourceStatus::Occupied)]));
    set_stat("booked", QString::number(counts[static_cast<int>(gc::ResourceStatus::Booked)]));
    set_stat("maintenance", QString::number(counts[static_cast<int>(gc::ResourceStatus::Maintenance)]));

    auto bookings = visible_bookings();
    const gc::Minutes day_start = now / gc::kMinutesPerDay * gc::kMinutesPerDay;
    const gc::TimeSlot today(day_start, gc::kMinutesPerDay);
    int today_count = 0;
    for (const auto& booking : bookings) {
        if (booking->slot().overlaps(today)) {
            ++today_count;
        }
    }
    set_stat("today", QString::number(today_count));
    set_stat("mine", QString::number(bookings.size()));
    if (!is_client()) {
        int clients = 0;
        for (const auto& user : center_.users().values()) {
            if (user->get_role() == gc::Role::Client) {
                ++clients;
            }
        }
        set_stat("clients", QString::number(clients));
    }
    if (is_owner()) {
        set_stat("revenue", money_text(center_.transactions().total_revenue()));
    }

    // Ближайшие (ещё не закончившиеся) брони по возрастанию времени начала.
    bookings.erase(std::remove_if(bookings.begin(), bookings.end(),
                                  [now](const std::shared_ptr<gc::Booking>& booking) {
                                      return booking->slot().end() <= now;
                                  }),
                   bookings.end());
    std::sort(bookings.begin(), bookings.end(),
              [](const std::shared_ptr<gc::Booking>& a, const std::shared_ptr<gc::Booking>& b) {
                  return a->slot().start() < b->slot().start();
              });
    const std::size_t rows = std::min<std::size_t>(bookings.size(), 8);
    upcoming_table_->setRowCount(static_cast<int>(rows));
    for (std::size_t i = 0; i < rows; ++i) {
        const auto& booking = bookings[i];
        const int row = static_cast<int>(i);
        const auto owner = center_.users().find(booking->client_id());
        set_cell(upcoming_table_, row, 0, qs(gc::timeutil::format_datetime(booking->slot().start())));
        set_cell(upcoming_table_, row, 1, qs(booking->title()));
        set_cell(upcoming_table_, row, 2, booking_resource_names(*booking));
        set_cell(upcoming_table_, row, 3, owner ? qs(owner->full_name()) : QString::number(booking->client_id()));
    }
}

void MainWindow::refresh_hall_map() {
    const gc::Minutes now = gc::timeutil::now_minutes();
    center_.refresh_statuses(now);
    for (const auto& entry : zone_buttons_) {
        const auto resource = center_.resources().find(entry.first);
        if (!resource) {
            continue;
        }
        const gc::ResourceStatus status = center_.get_device_status(entry.first, now);
        entry.second->setText(qs(resource->name()) + "\n" + qs(gc::to_string(status)));
        entry.second->setStyleSheet(zone_button_style(status));
    }
}

void MainWindow::refresh_bookings() {
    const auto shown = visible_bookings();
    bookings_table_->setRowCount(static_cast<int>(shown.size()));
    for (std::size_t i = 0; i < shown.size(); ++i) {
        const auto& booking = shown[i];
        const int row = static_cast<int>(i);
        const auto owner = center_.users().find(booking->client_id());

        set_cell(bookings_table_, row, 0, QString::number(booking->id()));
        set_cell(bookings_table_, row, 1, qs(booking->title()));
        set_cell(bookings_table_, row, 2, owner ? qs(owner->login()) : QString::number(booking->client_id()));
        set_cell(bookings_table_, row, 3, booking_resource_names(*booking));
        set_cell(bookings_table_, row, 4,
                 qs(gc::timeutil::format_datetime(booking->slot().start())) + " — " +
                     qs(gc::timeutil::format_datetime(booking->slot().end())));
        set_cell(bookings_table_, row, 5, money_text(booking->price()));
    }
}

void MainWindow::refresh_clients() {
    const gc::Minutes now = gc::timeutil::now_minutes();
    std::vector<std::shared_ptr<gc::Client>> clients;
    for (const auto& user : center_.users().values()) {
        if (auto client = std::dynamic_pointer_cast<gc::Client>(user)) {
            clients.push_back(client);
        }
    }
    clients_table_->setRowCount(static_cast<int>(clients.size()));
    for (std::size_t i = 0; i < clients.size(); ++i) {
        const auto& client = clients[i];
        const int row = static_cast<int>(i);
        set_cell(clients_table_, row, 0, QString::number(client->id()));
        set_cell(clients_table_, row, 1, qs(client->login()));
        set_cell(clients_table_, row, 2, qs(client->full_name()));
        set_cell(clients_table_, row, 3, QString::number(client->age_on(now / gc::kMinutesPerDay)));
        set_cell(clients_table_, row, 4, money_text(client->balance()));
        set_cell(clients_table_, row, 5, QString::number(client->discount_percent()) + "%");
    }
}

void MainWindow::refresh_transactions() {
    const auto entries = center_.transactions().entries().values();
    transactions_table_->setRowCount(static_cast<int>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        const int row = static_cast<int>(i);
        const auto client = center_.users().find(entry->client_id());
        set_cell(transactions_table_, row, 0, QString::number(entry->id()));
        set_cell(transactions_table_, row, 1, qs(gc::timeutil::format_datetime(entry->timestamp())));
        set_cell(transactions_table_, row, 2, client ? qs(client->login()) : QString::number(entry->client_id()));
        set_cell(transactions_table_, row, 3, qs(gc::to_string(entry->type())));
        set_cell(transactions_table_, row, 4, money_text(entry->amount()));
        set_cell(transactions_table_, row, 5, qs(entry->description()));
    }
    revenue_label_->setText("Выручка (списания − возвраты): " + money_text(center_.transactions().total_revenue()));
}

// ------------------------------------------------------------------ обработчики

void MainWindow::on_zone_clicked(int resource_id) {
    run_guarded([this, resource_id] {
        const auto resource = center_.resources().get(resource_id);
        const gc::Minutes now = gc::timeutil::now_minutes();
        const gc::ResourceStatus status = center_.get_device_status(resource_id, now);

        QMessageBox box(this);
        box.setWindowTitle(qs(resource->name()));
        box.setText(qs(resource->describe()) + "\n\nТекущий статус: " + qs(gc::to_string(status)));

        QPushButton* first = nullptr;
        QPushButton* second = nullptr;
        if (is_client()) {
            first = box.addButton("Начать сеанс", QMessageBox::ActionRole);
            second = box.addButton("Забронировать", QMessageBox::ActionRole);
        } else {
            first = box.addButton(resource->is_available() ? "На обслуживание" : "Вернуть в работу",
                                  QMessageBox::ActionRole);
            second = box.addButton("Освободить", QMessageBox::ActionRole);
        }
        box.addButton("Закрыть", QMessageBox::RejectRole);
        box.exec();

        const QAbstractButton* clicked = box.clickedButton();
        if (clicked == first && is_client()) {
            bool ok = false;
            const int minutes = QInputDialog::getInt(this, "Сеанс", "Длительность, мин:", 60, 1, 600, 15, &ok);
            if (!ok) {
                return;
            }
            int guests = 1;
            if (resource->capacity() > 1) {
                guests = QInputDialog::getInt(this, "Сеанс", "Участников:", 1, 1, resource->capacity(), 1, &ok);
                if (!ok) {
                    return;
                }
            }
            const gc::Money cost = center_.start_session(user_->id(), resource_id, minutes, guests, now);
            commit();
            refresh_all();
            QMessageBox::information(this, "Сеанс начат", "Списано: " + money_text(cost));
        } else if (clicked == second && is_client()) {
            on_book_clicked(resource_id);
        } else if (clicked == first) {
            center_.set_zone_maintenance(resource_id, resource->is_available());
            commit();
            refresh_all();
        } else if (clicked == second) {
            resource->release();
            commit();
            refresh_all();
        }
    });
}

void MainWindow::on_book_clicked(int preselected_resource_id) {
    run_guarded([this, preselected_resource_id] {
        BookingDialog dialog(center_, preselected_resource_id, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        const auto booking = center_.create_booking(user_->id(), dialog.resource_id(),
                                                    gc::TimeSlot(dialog.start(), dialog.minutes()), dialog.guests(),
                                                    gc::timeutil::now_minutes());
        commit();
        refresh_all();
        QMessageBox::information(this, "Бронь создана",
                                 "Бронь №" + QString::number(booking->id()) + ", списано: " +
                                     money_text(booking->price()));
    });
}

void MainWindow::on_deposit_clicked() {
    run_guarded([this] {
        bool ok = false;
        const double amount =
            QInputDialog::getDouble(this, "Пополнение баланса", "Сумма, BYN:", 10.0, 0.01, 100000.0, 2, &ok);
        if (!ok) {
            return;
        }
        center_.deposit(user_->id(), gc::Money::from_double(amount), gc::timeutil::now_minutes());
        commit();
        refresh_all();
    });
}

void MainWindow::on_cancel_booking_clicked() {
    run_guarded([this] {
        const int id = selected_id(bookings_table_);
        if (id < 0) {
            QMessageBox::information(this, "Отмена брони", "Выберите бронь в таблице.");
            return;
        }
        const gc::Money refund =
            center_.cancel_booking(id, is_client() ? user_->id() : -1, gc::timeutil::now_minutes());
        commit();
        refresh_all();
        QMessageBox::information(this, "Бронь отменена", "Возврат: " + money_text(refund));
    });
}

void MainWindow::on_generate_event_clicked() {
    run_guarded([this] {
        gc::EventRequest request;
        request.guests = guests_spin_->value();
        request.avg_age = age_spin_->value();
        request.budget = gc::Money::from_double(budget_spin_->value());
        request.start_minute = to_minutes(start_edit_->dateTime());
        request.slots_count = stages_spin_->value();

        generated_timeline_.reset();
        book_event_button_->setEnabled(false);
        timeline_table_->setRowCount(0);
        timeline_summary_->clear();

        const gc::EventTimeline timeline = center_.generate_event_timeline(request);
        generated_request_ = request;
        generated_timeline_ = timeline;

        timeline_table_->setRowCount(static_cast<int>(timeline.items.size()));
        for (std::size_t i = 0; i < timeline.items.size(); ++i) {
            const gc::TimelineItem& item = timeline.items[i];
            const int row = static_cast<int>(i);
            QString resources = qs(center_.resources().get(item.resource_ids.front())->name());
            if (item.resource_ids.size() > 1) {
                resources += " и ещё " + QString::number(item.resource_ids.size() - 1);
            }
            set_cell(timeline_table_, row, 0,
                     qs(gc::timeutil::format_datetime(item.slot.start())) + " (" +
                         QString::number(item.slot.duration()) + " мин)");
            set_cell(timeline_table_, row, 1, qs(item.activity));
            set_cell(timeline_table_, row, 2, resources);
            set_cell(timeline_table_, row, 3, money_text(item.cost));
        }
        timeline_summary_->setText("Без скидки: " + money_text(timeline.base_total) + "   ·   скидка " +
                                   QString::number(timeline.discount_percent) + "%   ·   к оплате: " +
                                   money_text(timeline.total));
        book_event_button_->setEnabled(true);
    });
}

void MainWindow::on_book_event_clicked() {
    run_guarded([this] {
        if (!generated_timeline_) {
            return;
        }
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Мероприятие", "Название мероприятия:",
                                                   QLineEdit::Normal, "День рождения", &ok);
        if (!ok || name.trimmed().isEmpty()) {
            return;
        }
        const auto booking = center_.create_event_booking(user_->id(), name.toStdString(), *generated_timeline_,
                                                          generated_request_.guests, generated_request_.avg_age,
                                                          gc::timeutil::now_minutes());
        generated_timeline_.reset();
        book_event_button_->setEnabled(false);
        commit();
        refresh_all();
        QMessageBox::information(this, "Мероприятие забронировано",
                                 "Бронь №" + QString::number(booking->id()) + ", списано: " +
                                     money_text(booking->price()));
    });
}

void MainWindow::on_client_deposit_clicked() {
    run_guarded([this] {
        const int id = selected_id(clients_table_);
        if (id < 0) {
            QMessageBox::information(this, "Пополнение", "Выберите клиента в таблице.");
            return;
        }
        bool ok = false;
        const double amount =
            QInputDialog::getDouble(this, "Пополнение счёта", "Сумма, BYN:", 10.0, 0.01, 100000.0, 2, &ok);
        if (!ok) {
            return;
        }
        center_.deposit(id, gc::Money::from_double(amount), gc::timeutil::now_minutes());
        commit();
        refresh_all();
    });
}

void MainWindow::on_client_discount_clicked() {
    run_guarded([this] {
        const int id = selected_id(clients_table_);
        if (id < 0) {
            QMessageBox::information(this, "Скидка", "Выберите клиента в таблице.");
            return;
        }
        bool ok = false;
        const int percent = QInputDialog::getInt(this, "Скидка клиента", "Скидка, % (0-50):", 0, 0, 50, 1, &ok);
        if (!ok) {
            return;
        }
        center_.get_client(id)->set_discount_percent(percent);
        commit();
        refresh_all();
    });
}

// ------------------------------------------------------------------ вспомогательное

void MainWindow::run_guarded(const std::function<void()>& action) {
    try {
        action();
    } catch (const gc::GameCenterException& error) {
        QMessageBox::warning(this, "Ошибка", qs(error.what()));
    }
}

void MainWindow::commit() {
    try {
        center_.save_all(data_dir_);
    } catch (const std::exception& error) {
        QMessageBox::warning(this, "Ошибка сохранения", qs(error.what()));
    }
}

int MainWindow::selected_id(QTableWidget* table) {
    const int row = table->currentRow();
    if (row < 0 || table->item(row, 0) == nullptr) {
        return -1;
    }
    return table->item(row, 0)->text().toInt();
}
