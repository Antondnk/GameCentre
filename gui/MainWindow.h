#pragma once

#include <QMainWindow>
#include <QString>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "GameCenter.h"

class QDateEdit;
class QDateTimeEdit;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTimer;
class QVBoxLayout;
class TimelineView;

// Главное окно: боковое меню + страницы. Набор страниц зависит от роли пользователя.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(gc::GameCenter& center, std::shared_ptr<gc::User> user, std::string data_dir,
               QWidget* parent = nullptr);

    bool logout_requested() const { return logout_requested_; }

private:
    // построение интерфейса
    void build_header(QVBoxLayout* content_layout);
    void add_page(const QString& title, QWidget* page);
    void show_page(int index);
    void build_dashboard_page();
    void build_map_page();
    void build_timeline_page();
    void build_event_page();
    void build_clients_page();
    void build_finance_page();

    // обновление данных на экране
    void refresh_all();
    void refresh_header();
    void refresh_dashboard();
    void refresh_hall_map();
    void refresh_bookings();
    void refresh_clients();
    void refresh_transactions();

    // обработчики
    void on_zone_clicked(int resource_id);
    void on_book_clicked(int preselected_resource_id);
    void on_deposit_clicked();
    void on_cancel_booking_clicked();
    void on_generate_event_clicked();
    void on_book_event_clicked();
    void on_client_deposit_clicked();
    void on_client_discount_clicked();

    // вспомогательное
    void run_guarded(const std::function<void()>& action);
    void commit();
    int selected_id(QTableWidget* table);
    void set_stat(const std::string& key, const QString& text);
    std::vector<std::shared_ptr<gc::Booking>> visible_bookings() const;
    QString booking_resource_names(const gc::Booking& booking) const;
    bool is_client() const { return user_->get_role() == gc::Role::Client; }
    bool is_owner() const { return user_->get_role() == gc::Role::Owner; }

    gc::GameCenter& center_;
    std::shared_ptr<gc::User> user_;
    std::string data_dir_;
    bool logout_requested_ = false;

    // каркас
    QFrame* sidebar_ = nullptr;
    QVBoxLayout* nav_layout_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QLabel* page_title_ = nullptr;
    QLabel* user_label_ = nullptr;
    QLabel* balance_label_ = nullptr;
    std::vector<QPushButton*> nav_buttons_;
    std::vector<QString> page_titles_;

    // рабочий стол
    std::map<std::string, QLabel*> stat_values_;
    QTableWidget* upcoming_table_ = nullptr;

    // карта зала
    std::map<int, QPushButton*> zone_buttons_;

    // бронирование
    TimelineView* timeline_ = nullptr;
    QDateEdit* timeline_date_ = nullptr;
    QTableWidget* bookings_table_ = nullptr;

    // конструктор праздника
    QSpinBox* guests_spin_ = nullptr;
    QSpinBox* age_spin_ = nullptr;
    QDoubleSpinBox* budget_spin_ = nullptr;
    QDateTimeEdit* start_edit_ = nullptr;
    QSpinBox* stages_spin_ = nullptr;
    QTableWidget* timeline_table_ = nullptr;
    QLabel* timeline_summary_ = nullptr;
    QPushButton* book_event_button_ = nullptr;
    std::optional<gc::EventTimeline> generated_timeline_;
    gc::EventRequest generated_request_;

    // персонал / владелец
    QTableWidget* clients_table_ = nullptr;
    QTableWidget* transactions_table_ = nullptr;
    QLabel* revenue_label_ = nullptr;

    QTimer* timer_ = nullptr;
};
