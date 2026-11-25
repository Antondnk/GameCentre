// Страница настроек: тема, акцент, размер шрифта, название клуба, резервная копия данных.
#include "MainWindow.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QUrl>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "GuiUtils.h"
#include "Icons.h"
#include "Theme.h"

void MainWindow::build_settings_page() {
    auto* page = new QWidget(stack_);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(make_section_title("Оформление и поведение", page));
    auto* form = new QFormLayout();

    theme_combo_ = new QComboBox(page);
    for (const Palette& palette : theme_presets()) {
        theme_combo_->addItem(palette.name);
    }
    theme_combo_->setCurrentText(settings_.theme);
    form->addRow("Тема:", theme_combo_);

    auto* accent_row = new QHBoxLayout();
    accent_button_ = new QPushButton("Выбрать цвет…", page);
    auto* reset = new QPushButton("Сбросить", page);
    accent_row->addWidget(accent_button_);
    accent_row->addWidget(reset);
    accent_row->addStretch();
    form->addRow("Акцентный цвет:", accent_row);
    pending_accent_ = settings_.accent;
    if (!pending_accent_.isEmpty()) {
        accent_button_->setText(pending_accent_);
    }

    font_spin_ = new QSpinBox(page);
    font_spin_->setRange(10, 20);
    font_spin_->setValue(settings_.font_px);
    font_spin_->setSuffix(" px");
    form->addRow("Размер шрифта:", font_spin_);

    refresh_spin_ = new QSpinBox(page);
    refresh_spin_->setRange(2, 60);
    refresh_spin_->setValue(settings_.refresh_sec);
    refresh_spin_->setSuffix(" сек");
    form->addRow("Автообновление карты:", refresh_spin_);

    club_edit_ = new QLineEdit(settings_.club_name, page);
    if (is_owner()) {
        form->addRow("Название клуба:", club_edit_);
    } else {
        club_edit_->hide();  // название клуба меняет только владелец
    }
    layout->addLayout(form);

    auto* apply = new QPushButton("Применить и сохранить", page);
    apply->setObjectName("accent");
    layout->addWidget(apply, 0, Qt::AlignLeft);

    layout->addSpacing(12);
    auto* data_title = make_section_title("Данные", page);
    layout->addWidget(data_title);
    auto* path_label = new QLabel("Папка данных: " + QDir(QString::fromStdString(data_dir_)).absolutePath(), page);
    path_label->setObjectName("muted");
    path_label->setWordWrap(true);
    path_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(path_label);

    auto* data_buttons = new QHBoxLayout();
    auto* open_folder = new QPushButton("Открыть папку с данными", page);
    auto* backup = new QPushButton("Создать резервную копию", page);
    data_buttons->addWidget(open_folder);
    data_buttons->addWidget(backup);
    data_buttons->addStretch();
    layout->addLayout(data_buttons);
    layout->addStretch();
    if (!is_owner()) {  // файлы данных (в т.ч. хэши паролей) - только владельцу
        data_title->hide();
        path_label->hide();
        open_folder->hide();
        backup->hide();
    }

    connect(accent_button_, &QPushButton::clicked, this, [this] { on_pick_accent(); });
    connect(reset, &QPushButton::clicked, this, [this] { on_reset_accent(); });
    connect(apply, &QPushButton::clicked, this, [this] { on_apply_settings(); });
    connect(open_folder, &QPushButton::clicked, this, [this] { on_open_data_folder(); });
    connect(backup, &QPushButton::clicked, this, [this] { on_backup_data(); });
    add_page("Настройки", "SETTINGS", page);
}

void MainWindow::on_pick_accent() {
    const QColor start = pending_accent_.isEmpty() ? QColor(resolve_palette(settings_).accent) : QColor(pending_accent_);
    const QColor color = QColorDialog::getColor(start, this, "Акцентный цвет");
    if (color.isValid()) {
        pending_accent_ = color.name();
        accent_button_->setText(pending_accent_);
    }
}

void MainWindow::on_reset_accent() {
    pending_accent_.clear();
    accent_button_->setText("Выбрать цвет…");
}

void MainWindow::on_apply_settings() {
    settings_.theme = theme_combo_->currentText();
    settings_.accent = pending_accent_;
    settings_.font_px = font_spin_->value();
    settings_.refresh_sec = refresh_spin_->value();
    if (is_owner()) {
        settings_.club_name =
            club_edit_->text().trimmed().isEmpty() ? QString("GAME CENTER") : club_edit_->text().trimmed();
    }

    QDir().mkpath(QString::fromStdString(data_dir_));
    settings_.save(settings_path_);
    apply_theme(settings_);

    logo_label_->setText(settings_.club_name);
    timer_->start(settings_.refresh_sec * 1000);
    zone_state_cache_.clear();  // принудительно перекрасить кнопки зон
    update_nav_icons();
    refresh_all();
    if (!is_client()) {
        note("Изменены настройки оформления");
        commit();
    }
}

void MainWindow::on_open_data_folder() {
    const QString path = QDir(QString::fromStdString(data_dir_)).absolutePath();
    QDir().mkpath(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::on_backup_data() {
    const QDir source(QString::fromStdString(data_dir_));
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    const QString target_path = source.absoluteFilePath("backup_" + stamp);
    if (!QDir().mkpath(target_path)) {
        QMessageBox::warning(this, "Резервная копия", "Не удалось создать папку копии.");
        return;
    }
    int copied = 0;
    for (const QFileInfo& info : source.entryInfoList(QStringList() << "*.txt" << "*.log" << "*.ini", QDir::Files)) {
        if (QFile::copy(info.absoluteFilePath(), target_path + "/" + info.fileName())) {
            ++copied;
        }
    }
    note("Резервная копия данных: " + target_path);
    QMessageBox::information(this, "Резервная копия",
                             "Скопировано файлов: " + QString::number(copied) + "\n" + target_path);
}
