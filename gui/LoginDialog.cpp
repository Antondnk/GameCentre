#include "LoginDialog.h"

#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "Exceptions.h"
#include "TimeUtils.h"

LoginDialog::LoginDialog(gc::GameCenter& center, std::function<void()> commit, QWidget* parent)
    : QDialog(parent), center_(center), commit_(std::move(commit)) {
    setWindowTitle("Игровой центр — вход");
    setMinimumWidth(360);

    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("Информационная система игрового центра", this);
    title->setStyleSheet("font-size: 15px; font-weight: bold;");
    title->setWordWrap(true);
    layout->addWidget(title);

    auto* form = new QFormLayout();
    login_edit_ = new QLineEdit(this);
    password_edit_ = new QLineEdit(this);
    password_edit_->setEchoMode(QLineEdit::Password);
    form->addRow("Логин:", login_edit_);
    form->addRow("Пароль:", password_edit_);
    layout->addLayout(form);

    error_label_ = new QLabel(this);
    error_label_->setStyleSheet("color: #c62828;");
    layout->addWidget(error_label_);

    auto* hint = new QLabel("Тестовые записи: owner/owner123, admin/admin123, client/client123, kid/kid12345", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: gray; font-size: 11px;");
    layout->addWidget(hint);

    auto* buttons = new QHBoxLayout();
    auto* login_button = new QPushButton("Войти", this);
    auto* register_button = new QPushButton("Регистрация", this);
    auto* exit_button = new QPushButton("Выход", this);
    login_button->setDefault(true);
    buttons->addWidget(login_button);
    buttons->addWidget(register_button);
    buttons->addWidget(exit_button);
    layout->addLayout(buttons);

    connect(login_button, &QPushButton::clicked, this, [this] { try_login(); });
    connect(password_edit_, &QLineEdit::returnPressed, this, [this] { try_login(); });
    connect(register_button, &QPushButton::clicked, this, [this] { open_registration(); });
    connect(exit_button, &QPushButton::clicked, this, [this] { reject(); });
}

void LoginDialog::try_login() {
    try {
        user_ = center_.authenticate(login_edit_->text().toStdString(), password_edit_->text().toStdString());
        accept();
    } catch (const gc::GameCenterException& error) {
        error_label_->setText(QString::fromUtf8(error.what()));
        password_edit_->clear();
    }
}

void LoginDialog::open_registration() {
    QDialog dialog(this);
    dialog.setWindowTitle("Регистрация клиента");
    auto* form = new QFormLayout(&dialog);

    auto* login = new QLineEdit(&dialog);
    auto* password = new QLineEdit(&dialog);
    password->setEchoMode(QLineEdit::Password);
    auto* full_name = new QLineEdit(&dialog);
    auto* birth = new QDateEdit(QDate(2000, 1, 1), &dialog);
    birth->setCalendarPopup(true);
    birth->setDisplayFormat("yyyy-MM-dd");
    birth->setMaximumDate(QDate::currentDate());

    form->addRow("Логин:", login);
    form->addRow("Пароль (мин. 4 символа):", password);
    form->addRow("ФИО:", full_name);
    form->addRow("Дата рождения:", birth);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    try {
        const QDate date = birth->date();
        const long long birth_days = gc::timeutil::days_from_civil(date.year(), static_cast<unsigned>(date.month()),
                                                                   static_cast<unsigned>(date.day()));
        center_.register_client(login->text().toStdString(), password->text().toStdString(),
                                full_name->text().toStdString(), birth_days);
        commit_();
        QMessageBox::information(this, "Регистрация", "Клиент зарегистрирован. Теперь можно войти.");
        login_edit_->setText(login->text());
        password_edit_->clear();
        error_label_->clear();
    } catch (const gc::GameCenterException& error) {
        QMessageBox::warning(this, "Ошибка", QString::fromUtf8(error.what()));
    }
}
