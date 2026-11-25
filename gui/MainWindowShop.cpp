// Магазин, склад и профиль клиента.
#include "MainWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "Dialogs.h"
#include "Exceptions.h"
#include "GuiUtils.h"
#include "Icons.h"
#include "Theme.h"
#include "TimeUtils.h"

// ------------------------------------------------------------------ магазин

void MainWindow::build_shop_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    shop_info_ = new QLabel(page);
    shop_info_->setObjectName("muted");
    layout->addWidget(shop_info_);

    shop_list_ = new QListWidget(page);
    shop_list_->setViewMode(QListView::IconMode);
    shop_list_->setResizeMode(QListView::Adjust);
    shop_list_->setMovement(QListView::Static);
    shop_list_->setIconSize(QSize(56, 56));
    shop_list_->setGridSize(QSize(190, 120));
    shop_list_->setWordWrap(true);
    shop_list_->setSpacing(8);
    layout->addWidget(shop_list_, 1);

    connect(shop_list_, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        on_shop_item_clicked(item->data(Qt::UserRole).toInt());
    });
    connect(shop_list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        on_shop_item_clicked(item->data(Qt::UserRole).toInt());
    });
    add_page("Магазин и бар", "CART", page);
}

void MainWindow::refresh_shop() {
    const Palette& palette = current_palette();
    const gc::Client& client = *center_.get_client(user_->id());
    const int discount = center_.effective_discount(client);
    shop_info_->setText("Нажмите на товар, чтобы купить. Ваша скидка: " + QString::number(discount) +
                        "%  ·  баланс: " + money_text(client.balance()));

    shop_list_->clear();
    for (const auto& product : center_.products().values()) {
        const bool available = product->stock() > 0;
        auto* item = new QListWidgetItem(
            make_icon(icon_tag_for_category(product->category()), QColor(available ? palette.accent : palette.muted), 56),
            qs(product->name()) + "\n" + money_text(product->price()) +
                (available ? "  ·  " + QString::number(product->stock()) + " шт." : QString("  ·  нет в наличии")));
        item->setData(Qt::UserRole, product->id());
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
        if (!available) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
        shop_list_->addItem(item);
    }
}

void MainWindow::on_shop_item_clicked(int product_id) {
    run_guarded([this, product_id] {
        const auto product = center_.products().get(product_id);
        bool ok = false;
        const int max_qty = std::max(1, product->stock());
        const int quantity = QInputDialog::getInt(this, qs(product->name()),
                                                  "Цена " + money_text(product->price()) + " за шт.\nКоличество:", 1, 1,
                                                  max_qty, 1, &ok);
        if (!ok) {
            return;
        }
        const gc::Money total = center_.buy_product(user_->id(), product_id, quantity, gc::timeutil::now_minutes());
        commit();
        refresh_all();
        QMessageBox::information(this, "Покупка", "Списано: " + money_text(total));
    });
}

// ------------------------------------------------------------------ склад

void MainWindow::build_stock_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    stock_table_ = make_table(page, {"ID", "Название", "Категория", "Цена", "Остаток"});
    layout->addWidget(stock_table_, 1);

    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("+ Новый товар", page);
    add->setObjectName("accent");
    auto* restock = new QPushButton("Пополнить склад", page);
    auto* remove = new QPushButton("Удалить товар", page);
    remove->setObjectName("danger");
    remove->setVisible(is_owner());  // удалять товары может только владелец
    connect(add, &QPushButton::clicked, this, [this] { on_add_product(); });
    connect(restock, &QPushButton::clicked, this, [this] { on_restock_product(); });
    connect(remove, &QPushButton::clicked, this, [this] { on_remove_product(); });
    buttons->addStretch();
    buttons->addWidget(add);
    buttons->addWidget(restock);
    buttons->addWidget(remove);
    layout->addLayout(buttons);
    add_page("Склад и товары", "BOX", page);
}

void MainWindow::refresh_stock() {
    const auto products = center_.products().values();
    const int selected = selected_id(stock_table_);
    stock_table_->setRowCount(static_cast<int>(products.size()));
    for (std::size_t i = 0; i < products.size(); ++i) {
        const auto& product = products[i];
        const int row = static_cast<int>(i);
        set_cell(stock_table_, row, 0, QString::number(product->id()));
        set_cell(stock_table_, row, 1, qs(product->name()));
        set_cell(stock_table_, row, 2, qs(product->category()));
        set_cell(stock_table_, row, 3, money_text(product->price()));
        set_cell(stock_table_, row, 4, QString::number(product->stock()));
        if (product->id() == selected) {
            stock_table_->selectRow(row);
        }
    }
}

void MainWindow::on_add_product() {
    run_guarded([this] {
        ProductDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        center_.add_product(dialog.name(), dialog.category(), dialog.price(), dialog.stock());
        note(qs("Добавлен товар: " + dialog.name()));
        commit();
        refresh_all();
    });
}

void MainWindow::on_restock_product() {
    run_guarded([this] {
        const int id = selected_id(stock_table_);
        if (id < 0) {
            QMessageBox::information(this, "Склад", "Выберите товар в таблице.");
            return;
        }
        bool ok = false;
        const int quantity = QInputDialog::getInt(this, "Пополнение склада", "Сколько добавить:", 10, 1, 100000, 1, &ok);
        if (!ok) {
            return;
        }
        center_.restock_product(id, quantity);
        note("Пополнен склад: товар #" + QString::number(id) + " +" + QString::number(quantity));
        commit();
        refresh_all();
    });
}

void MainWindow::on_remove_product() {
    run_guarded([this] {
        const int id = selected_id(stock_table_);
        if (id < 0) {
            QMessageBox::information(this, "Склад", "Выберите товар в таблице.");
            return;
        }
        if (QMessageBox::question(this, "Удаление", "Удалить выбранный товар?") != QMessageBox::Yes) {
            return;
        }
        center_.remove_product(id);
        note("Удалён товар #" + QString::number(id));
        commit();
        refresh_all();
    });
}

// ------------------------------------------------------------------ профиль

void MainWindow::build_profile_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(make_section_title("Программа лояльности", page));
    profile_progress_ = new QProgressBar(page);
    profile_progress_->setRange(0, 100);
    layout->addWidget(profile_progress_);
    profile_next_ = new QLabel(page);
    profile_next_->setObjectName("muted");
    layout->addWidget(profile_next_);

    layout->addWidget(make_section_title("Мои операции", page));
    history_table_ = make_table(page, {"Время", "Тип", "Категория", "Сумма", "Описание"});
    layout->addWidget(history_table_, 1);
    add_page("Мой профиль", "STAR", page);
}

void MainWindow::refresh_profile() {
    const gc::LoyaltyLevel level = center_.loyalty_level(user_->id());
    if (level.next_threshold.cents() > 0) {
        const double percent = 100.0 * static_cast<double>(level.spent.cents()) /
                               static_cast<double>(level.next_threshold.cents());
        profile_progress_->setValue(std::min(100, static_cast<int>(percent)));
        profile_progress_->setFormat(qs(level.name) + " · скидка " + QString::number(level.discount_percent) + "%  (%p%)");
        profile_next_->setText("Потрачено " + money_text(level.spent) + " из " + money_text(level.next_threshold) +
                               " до уровня «" + qs(level.next_name) + "»");
    } else {
        profile_progress_->setValue(100);
        profile_progress_->setFormat(qs(level.name) + " · скидка " + QString::number(level.discount_percent) + "% (максимум)");
        profile_next_->setText("Потрачено " + money_text(level.spent) + ". Вы достигли максимального уровня!");
    }

    std::vector<std::shared_ptr<gc::Transaction>> mine;
    for (const auto& entry : center_.transactions().entries().values()) {
        if (entry->client_id() == user_->id()) {
            mine.push_back(entry);
        }
    }
    std::reverse(mine.begin(), mine.end());
    history_table_->setRowCount(static_cast<int>(mine.size()));
    for (std::size_t i = 0; i < mine.size(); ++i) {
        const int row = static_cast<int>(i);
        set_cell(history_table_, row, 0, qs(gc::timeutil::format_datetime(mine[i]->timestamp())));
        set_cell(history_table_, row, 1, qs(gc::to_string(mine[i]->type())));
        set_cell(history_table_, row, 2, qs(mine[i]->category()));
        set_cell(history_table_, row, 3, money_text(mine[i]->amount()));
        set_cell(history_table_, row, 4, qs(mine[i]->description()));
    }
}
