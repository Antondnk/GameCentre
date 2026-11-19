#pragma once

#include <QDialog>

#include <functional>
#include <memory>

#include "GameCenter.h"

class QLabel;
class QLineEdit;

// Окно входа: авторизация и регистрация нового клиента.
class LoginDialog : public QDialog {
    Q_OBJECT

public:
    LoginDialog(gc::GameCenter& center, std::function<void()> commit, QWidget* parent = nullptr);

    std::shared_ptr<gc::User> user() const { return user_; }

private:
    void try_login();
    void open_registration();

    gc::GameCenter& center_;
    std::function<void()> commit_;
    std::shared_ptr<gc::User> user_;
    QLineEdit* login_edit_ = nullptr;
    QLineEdit* password_edit_ = nullptr;
    QLabel* error_label_ = nullptr;
};
