// Страницы персонала и владельца: клиенты, ресурсы, аналитика, отчёты, журнал, финансы.
#include "MainWindow.h"

#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QFont>
#include <QScrollArea>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "Analytics.h"
#include "ChartWidget.h"
#include "Dialogs.h"
#include "Exceptions.h"
#include "GuiUtils.h"
#include "Report.h"
#include "TimeUtils.h"
#include "TimelineView.h"

namespace {

QString bar_text(double value) {
    return QString::number(value, 'f', 1);
}

}  // namespace

// ------------------------------------------------------------------ клиенты

void MainWindow::build_clients_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    clients_table_ = make_table(page, {"ID", "Логин", "ФИО", "Возраст", "Баланс", "Потрачено", "Уровень", "Скидка"});
    layout->addWidget(clients_table_, 1);

    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("+ Новый клиент", page);
    add->setObjectName("accent");
    auto* deposit = new QPushButton("Пополнить счёт", page);
    auto* discount = new QPushButton("Задать скидку", page);
    connect(add, &QPushButton::clicked, this, [this] { on_new_client(); });
    connect(deposit, &QPushButton::clicked, this, [this] { on_client_deposit_clicked(); });
    connect(discount, &QPushButton::clicked, this, [this] { on_client_discount_clicked(); });
    buttons->addStretch();
    buttons->addWidget(add);
    buttons->addWidget(deposit);
    buttons->addWidget(discount);
    layout->addLayout(buttons);
    add_page("Клиенты", "CLIENTS", page);
}

void MainWindow::refresh_clients() {
    const gc::Minutes now = gc::timeutil::now_minutes();
    const int selected = selected_id(clients_table_);
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
        const gc::LoyaltyLevel level = center_.loyalty_level(client->id());
        set_cell(clients_table_, row, 0, QString::number(client->id()));
        set_cell(clients_table_, row, 1, qs(client->login()));
        set_cell(clients_table_, row, 2, qs(client->full_name()));
        set_cell(clients_table_, row, 3, QString::number(client->age_on(now / gc::kMinutesPerDay)));
        set_cell(clients_table_, row, 4, money_text(client->balance()));
        set_cell(clients_table_, row, 5, money_text(level.spent));
        set_cell(clients_table_, row, 6, qs(level.name));
        set_cell(clients_table_, row, 7, QString::number(center_.effective_discount(*client)) + "%");
        if (client->id() == selected) {
            clients_table_->selectRow(row);
        }
    }
}

void MainWindow::on_new_client() {
    run_guarded([this] {
        ClientDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        center_.register_client(dialog.login(), dialog.password(), dialog.full_name(), dialog.birth_days());
        note("Зарегистрирован клиент: " + qs(dialog.login()));
        commit();
        refresh_all();
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
        note("Скидка клиента #" + QString::number(id) + ": " + QString::number(percent) + "%");
        commit();
        refresh_all();
    });
}

// ------------------------------------------------------------------ ресурсы

void MainWindow::build_resources_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* info = new QLabel(
        "Здесь можно добавлять новые зоны и устройства (ПК, лазертаг, VR, консоли, бильярд и др.), менять цены и "
        "отправлять ресурсы на обслуживание — без консоли.",
        page);
    info->setObjectName("muted");
    info->setWordWrap(true);
    layout->addWidget(info);

    resources_table_ = make_table(page, {"ID", "Название", "Тип", "Цена", "Возраст", "Вместимость", "Статус"});
    layout->addWidget(resources_table_, 1);

    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("+ Добавить ресурс", page);
    add->setObjectName("accent");
    auto* price = new QPushButton("Изменить цену", page);
    auto* maintenance = new QPushButton("Обслуживание вкл/выкл", page);
    auto* remove = new QPushButton("Удалить", page);
    remove->setObjectName("danger");
    remove->setVisible(is_owner());  // удалять ресурсы может только владелец
    connect(add, &QPushButton::clicked, this, [this] { on_add_resource(); });
    connect(price, &QPushButton::clicked, this, [this] { on_edit_price(); });
    connect(maintenance, &QPushButton::clicked, this, [this] { on_toggle_maintenance(); });
    connect(remove, &QPushButton::clicked, this, [this] { on_remove_resource(); });
    buttons->addStretch();
    buttons->addWidget(add);
    buttons->addWidget(price);
    buttons->addWidget(maintenance);
    buttons->addWidget(remove);
    layout->addLayout(buttons);
    add_page("Ресурсы", "SETTINGS", page);
}

void MainWindow::refresh_resources() {
    const gc::Minutes now = gc::timeutil::now_minutes();
    const int selected = selected_id(resources_table_);
    const auto resources = center_.resources().values();
    resources_table_->setRowCount(static_cast<int>(resources.size()));
    for (std::size_t i = 0; i < resources.size(); ++i) {
        const auto& resource = resources[i];
        const int row = static_cast<int>(i);
        set_cell(resources_table_, row, 0, QString::number(resource->id()));
        set_cell(resources_table_, row, 1, qs(resource->name()));
        set_cell(resources_table_, row, 2, qs(resource->kind()));
        set_cell(resources_table_, row, 3, money_text(resource->base_price()) + " / " + qs(resource->price_unit()));
        set_cell(resources_table_, row, 4,
                 QString::number(resource->min_age()) + "–" + QString::number(resource->max_age()));
        set_cell(resources_table_, row, 5, QString::number(resource->capacity()));
        set_cell(resources_table_, row, 6, qs(gc::to_string(center_.get_device_status(resource->id(), now))));
        if (resource->id() == selected) {
            resources_table_->selectRow(row);
        }
    }
}

void MainWindow::on_add_resource() {
    run_guarded([this] {
        AddResourceDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        const auto resource = dialog.create(center_.resources().next_id());
        center_.add_resource(resource);
        note("Добавлен ресурс: " + qs(resource->name()));
        commit();
        build_zone_buttons();
        refresh_all();
    });
}

void MainWindow::on_edit_price() {
    run_guarded([this] {
        const int id = selected_id(resources_table_);
        if (id < 0) {
            QMessageBox::information(this, "Цена", "Выберите ресурс в таблице.");
            return;
        }
        const auto resource = center_.resources().get(id);
        bool ok = false;
        const double price = QInputDialog::getDouble(this, "Новая цена", "Цена, BYN:",
                                                     static_cast<double>(resource->base_price().cents()) / 100.0, 0.0,
                                                     10000.0, 2, &ok);
        if (!ok) {
            return;
        }
        center_.set_resource_price(id, gc::Money::from_double(price));
        note("Цена ресурса " + qs(resource->name()) + ": " + QString::number(price, 'f', 2));
        commit();
        refresh_all();
    });
}

void MainWindow::on_toggle_maintenance() {
    run_guarded([this] {
        const int id = selected_id(resources_table_);
        if (id < 0) {
            QMessageBox::information(this, "Обслуживание", "Выберите ресурс в таблице.");
            return;
        }
        const auto resource = center_.resources().get(id);
        center_.set_zone_maintenance(id, resource->is_available());
        note("Обслуживание ресурса " + qs(resource->name()));
        commit();
        refresh_all();
    });
}

void MainWindow::on_remove_resource() {
    run_guarded([this] {
        const int id = selected_id(resources_table_);
        if (id < 0) {
            QMessageBox::information(this, "Удаление", "Выберите ресурс в таблице.");
            return;
        }
        if (QMessageBox::question(this, "Удаление", "Удалить выбранный ресурс?") != QMessageBox::Yes) {
            return;
        }
        center_.remove_resource(id);
        note("Удалён ресурс #" + QString::number(id));
        commit();
        build_zone_buttons();
        refresh_all();
    });
}

// ------------------------------------------------------------------ аналитика

void MainWindow::build_analytics_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* top = new QHBoxLayout();
    top->addWidget(new QLabel("Период:", page));
    analytics_days_ = new QComboBox(page);
    analytics_days_->addItem("7 дней", 7);
    analytics_days_->addItem("14 дней", 14);
    analytics_days_->addItem("30 дней", 30);
    top->addWidget(analytics_days_);
    top->addStretch();
    layout->addLayout(top);

    auto* grid = new QGridLayout();
    grid->setSpacing(12);
    const auto make_chart = [&](ChartWidget*& slot, const QString& title, ChartWidget::Orientation orientation) {
        slot = new ChartWidget(page);
        slot->set_title(title);
        slot->set_orientation(orientation);
        slot->setMinimumHeight(220);
    };
    make_chart(chart_days_, "Выручка по дням, BYN", ChartWidget::Orientation::Vertical);
    make_chart(chart_categories_, "Выручка по направлениям, BYN", ChartWidget::Orientation::Horizontal);
    make_chart(chart_resources_, "Загрузка ресурсов, часов брони", ChartWidget::Orientation::Horizontal);
    make_chart(chart_hours_, "В какое время бронируют", ChartWidget::Orientation::Vertical);
    make_chart(chart_clients_, "Топ клиентов по тратам, BYN", ChartWidget::Orientation::Horizontal);
    make_chart(chart_products_, "Популярные товары, шт.", ChartWidget::Orientation::Horizontal);
    grid->addWidget(chart_days_, 0, 0);
    grid->addWidget(chart_categories_, 0, 1);
    grid->addWidget(chart_resources_, 1, 0);
    grid->addWidget(chart_hours_, 1, 1);
    grid->addWidget(chart_clients_, 2, 0);
    grid->addWidget(chart_products_, 2, 1);

    auto* scroll_content = new QWidget(page);
    scroll_content->setLayout(grid);
    auto* scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setWidget(scroll_content);
    layout->addWidget(scroll, 1);

    connect(analytics_days_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { refresh_analytics(); });
    add_page("Аналитика", "CHART", page);
}

void MainWindow::refresh_analytics() {
    const int days = analytics_days_->currentData().toInt();
    const gc::AnalyticsReport report = gc::compute_analytics(center_, gc::timeutil::now_minutes(), days);

    std::vector<ChartWidget::Bar> bars;
    for (const gc::DailyValue& day : report.revenue_by_day) {
        const QDate date = QDate(1970, 1, 1).addDays(static_cast<int>(day.day));
        bars.push_back({date.toString("dd.MM"), day.value, bar_text(day.value)});
    }
    chart_days_->set_data(std::move(bars));

    bars.clear();
    for (const gc::NamedValue& item : report.revenue_by_category) {
        bars.push_back({qs(gc::category_title(item.name)), item.value, bar_text(item.value)});
    }
    chart_categories_->set_data(std::move(bars));

    bars.clear();
    for (const gc::NamedValue& item : report.booked_hours_by_resource) {
        bars.push_back({qs(item.name), item.value, bar_text(item.value) + " ч"});
    }
    chart_resources_->set_data(std::move(bars));

    bars.clear();
    for (int hour = 0; hour < 24; ++hour) {
        const int count = report.bookings_by_hour[static_cast<std::size_t>(hour)];
        bars.push_back({QString::number(hour), static_cast<double>(count), QString::number(count)});
    }
    chart_hours_->set_data(std::move(bars));

    bars.clear();
    for (const gc::NamedValue& item : report.top_clients) {
        bars.push_back({qs(item.name), item.value, bar_text(item.value)});
    }
    chart_clients_->set_data(std::move(bars));

    bars.clear();
    for (const gc::NamedValue& item : report.top_products) {
        bars.push_back({qs(item.name), item.value, QString::number(static_cast<int>(item.value))});
    }
    chart_products_->set_data(std::move(bars));
}

// ------------------------------------------------------------------ финансы

void MainWindow::build_finance_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    transactions_table_ = make_table(page, {"№", "Время", "Клиент", "Тип", "Категория", "Сумма", "Описание"});
    layout->addWidget(transactions_table_, 1);

    auto* bottom = new QHBoxLayout();
    revenue_label_ = new QLabel(page);
    revenue_label_->setObjectName("sectionTitle");
    bottom->addWidget(revenue_label_);
    bottom->addStretch();
    auto* csv = new QPushButton("Экспорт в CSV (Excel)", page);
    connect(csv, &QPushButton::clicked, this, [this] { on_export_csv(); });
    bottom->addWidget(csv);
    layout->addLayout(bottom);
    add_page("Финансы", "COINS", page);
}

void MainWindow::refresh_transactions() {
    auto entries = center_.transactions().entries().values();
    std::reverse(entries.begin(), entries.end());
    transactions_table_->setRowCount(static_cast<int>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        const int row = static_cast<int>(i);
        const auto client = center_.users().find(entry->client_id());
        set_cell(transactions_table_, row, 0, QString::number(entry->id()));
        set_cell(transactions_table_, row, 1, qs(gc::timeutil::format_datetime(entry->timestamp())));
        set_cell(transactions_table_, row, 2, client ? qs(client->login()) : QString::number(entry->client_id()));
        set_cell(transactions_table_, row, 3, qs(gc::to_string(entry->type())));
        set_cell(transactions_table_, row, 4, qs(gc::category_title(entry->category())));
        set_cell(transactions_table_, row, 5, money_text(entry->amount()));
        set_cell(transactions_table_, row, 6, qs(entry->description()));
    }
    revenue_label_->setText("Выручка (списания − возвраты): " + money_text(center_.transactions().total_revenue()));
}

void MainWindow::on_export_csv() {
    const QString path = QFileDialog::getSaveFileName(this, "Экспорт транзакций", data_path("transactions.csv"),
                                                      "CSV (*.csv)");
    if (path.isEmpty()) {
        return;
    }
    if (!write_text_file(path, qs(gc::build_transactions_csv(center_)))) {
        QMessageBox::warning(this, "Экспорт", "Не удалось записать файл.");
        return;
    }
    note("Экспорт транзакций: " + path);
    QMessageBox::information(this, "Экспорт", "Файл сохранён:\n" + path);
}

// ------------------------------------------------------------------ отчёты

void MainWindow::build_reports_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* buttons = new QHBoxLayout();
    auto* generate = new QPushButton("Сформировать отчёт", page);
    generate->setObjectName("accent");
    auto* save = new QPushButton("Сохранить в файл…", page);
    auto* notepad = new QPushButton("Открыть в Блокноте", page);
    auto* load = new QPushButton("Загрузить из файла…", page);
    connect(generate, &QPushButton::clicked, this, [this] { on_report_generate(); });
    connect(save, &QPushButton::clicked, this, [this] { on_report_save(); });
    connect(notepad, &QPushButton::clicked, this, [this] { on_report_open_notepad(); });
    connect(load, &QPushButton::clicked, this, [this] { on_report_load(); });
    buttons->addWidget(generate);
    buttons->addWidget(save);
    buttons->addWidget(notepad);
    buttons->addWidget(load);
    buttons->addStretch();
    layout->addLayout(buttons);

    report_edit_ = new QPlainTextEdit(page);
    report_edit_->setFont(QFont("Consolas", 10));
    report_edit_->setPlaceholderText("Нажмите «Сформировать отчёт» — здесь появится сводка по клубу.");
    layout->addWidget(report_edit_, 1);
    add_page("Отчёты", "REPORT", page);
}

void MainWindow::on_report_generate() {
    run_guarded([this] {
        report_edit_->setPlainText(qs(gc::build_text_report(center_, gc::timeutil::now_minutes())));
    });
}

void MainWindow::on_report_save() {
    if (report_edit_->toPlainText().isEmpty()) {
        on_report_generate();
    }
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm");
    const QString path = QFileDialog::getSaveFileName(this, "Сохранить отчёт", data_path("report_" + stamp + ".txt"),
                                                      "Текстовые файлы (*.txt)");
    if (path.isEmpty()) {
        return;
    }
    if (!write_text_file(path, report_edit_->toPlainText())) {
        QMessageBox::warning(this, "Отчёт", "Не удалось записать файл.");
        return;
    }
    note("Сохранён отчёт: " + path);
    QMessageBox::information(this, "Отчёт", "Отчёт сохранён:\n" + path);
}

void MainWindow::on_report_open_notepad() {
    if (report_edit_->toPlainText().isEmpty()) {
        on_report_generate();
    }
    QDir().mkpath(QString::fromStdString(data_dir_));
    const QString path = QDir(QString::fromStdString(data_dir_)).absoluteFilePath("report_latest.txt");
    if (!write_text_file(path, report_edit_->toPlainText())) {
        QMessageBox::warning(this, "Отчёт", "Не удалось записать файл.");
        return;
    }
#ifdef Q_OS_WIN
    QProcess::startDetached("notepad.exe", QStringList() << QDir::toNativeSeparators(path));
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}

void MainWindow::on_report_load() {
    const QString path = QFileDialog::getOpenFileName(this, "Загрузить отчёт", QString::fromStdString(data_dir_),
                                                      "Текстовые файлы (*.txt *.csv);;Все файлы (*)");
    if (path.isEmpty()) {
        return;
    }
    QString text;
    if (!read_text_file(path, &text)) {
        QMessageBox::warning(this, "Отчёт", "Не удалось прочитать файл.");
        return;
    }
    report_edit_->setPlainText(text);
}

// ------------------------------------------------------------------ журнал

void MainWindow::build_activity_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    activity_filter_ = new QLineEdit(page);
    activity_filter_->setPlaceholderText("Фильтр: введите часть текста (логин, ресурс, «Бронь», «Покупка»…)");
    layout->addWidget(activity_filter_);
    activity_edit_ = new QPlainTextEdit(page);
    activity_edit_->setReadOnly(true);
    activity_edit_->setFont(QFont("Consolas", 10));
    layout->addWidget(activity_edit_, 1);
    connect(activity_filter_, &QLineEdit::textChanged, this, [this](const QString&) { refresh_activity(); });
    add_page("Журнал действий", "LIST", page);
}

void MainWindow::refresh_activity() {
    const QString filter = activity_filter_->text().trimmed();
    QStringList lines;
    const auto& log = center_.activity_log();
    for (auto it = log.rbegin(); it != log.rend(); ++it) {
        const QString line = qs(*it);
        if (filter.isEmpty() || line.contains(filter, Qt::CaseInsensitive)) {
            lines << line;
        }
        if (lines.size() >= 500) {
            break;
        }
    }
    activity_edit_->setPlainText(lines.join("\n"));
}
