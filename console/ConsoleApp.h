#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "GameCenter.h"

namespace gc {

// Поток ввода закрыт (EOF) - штатное завершение программы.
class InputClosed : public std::exception {
public:
    const char* what() const noexcept override { return "input closed"; }
};

// Консольный интерфейс поверх GameCenter. Меню зависит от роли пользователя.
class ConsoleApp {
public:
    ConsoleApp(GameCenter& center, std::string data_dir, std::istream& in, std::ostream& out);

    void run();

private:
    struct MenuItem {
        int key;
        std::string title;
        std::function<void()> action;
    };

    // ввод
    std::string read_line(const std::string& prompt);
    int read_int(const std::string& prompt);
    Money read_money(const std::string& prompt);
    Minutes read_datetime(const std::string& prompt);  // пустая строка = сейчас
    bool confirm(const std::string& prompt);

    // меню
    void dispatch(const std::vector<MenuItem>& items);
    void run_guest_menu();
    void run_user_menu();
    std::vector<MenuItem> client_items();
    std::vector<MenuItem> staff_items(bool is_owner);

    // общие действия
    void login();
    void register_client();
    void logout();
    void quit();
    void show_hall_map();
    void show_catalog();
    void show_bookings(bool only_mine);
    void cancel_booking(bool as_staff);

    // клиент
    void show_profile();
    void deposit_self();
    void start_session();
    void book_resource();
    void plan_event();

    // персонал
    void change_status();
    void toggle_maintenance();
    void release_resource();
    void show_clients();
    void deposit_for_client();
    void set_client_discount();

    // владелец
    void show_transactions();
    void add_resource();
    void remove_resource();
    void create_staff();

    void print_timeline(const EventTimeline& timeline);
    std::shared_ptr<Client> current_client() const;
    static Minutes now();

    GameCenter& center_;
    std::string data_dir_;
    std::istream& in_;
    std::ostream& out_;
    std::shared_ptr<User> current_;
    bool running_ = true;
};

}  // namespace gc
