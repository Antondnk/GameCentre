#pragma once

#include <QMainWindow>
#include <QString>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AppSettings.h"
#include "GameCenter.h"

class ChartWidget;
class QComboBox;
class QDateEdit;
class QDateTimeEdit;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTimer;
class QVBoxLayout;
class TimelineView;

// Главное окно: боковое меню + страницы. Набор страниц зависит от роли пользователя.
// Реализация разнесена по файлам: MainWindow.cpp (каркас, рабочий стол, карта, брони, праздник),
// MainWindowShop.cpp (магазин, склад, профиль), MainWindowAdmin.cpp (клиенты, ресурсы, аналитика,
// отчёты, журнал, финансы), MainWindowSettings.cpp (настройки и оформление).
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(gc::GameCenter& center, std::shared_ptr<gc::User> user, std::string data_dir,
               QWidget* parent = nullptr);

    bool logout_requested() const { return logout_requested_; }

private:
    // ----- каркас (MainWindow.cpp) -----
    void build_header(QVBoxLayout* content_layout);
    void add_page(const QString& title, const QString& icon_tag, QWidget* page);
    void show_page(int index);
    void update_nav_icons();
    void build_dashboard_page();
    void build_map_page();
    void build_zone_buttons();
    void build_timeline_page();
    void build_event_page();

    void refresh_all();
    void refresh_header();
    void refresh_dashboard();
    void refresh_hall_map();
    void refresh_bookings();

    void on_zone_clicked(int resource_id);
    void on_book_clicked(int preselected_resource_id);
    void on_deposit_clicked();
    void on_cancel_booking_clicked();
    void on_generate_event_clicked();
    void on_book_event_clicked();

    // ----- магазин, склад, профиль (MainWindowShop.cpp) -----
    void build_shop_page();
    void build_stock_page();
    void build_profile_page();
    void refresh_shop();
    void refresh_stock();
    void refresh_profile();
    void on_shop_item_clicked(int product_id);
    void on_add_product();
    void on_restock_product();
    void on_remove_product();

    // ----- управление и аналитика (MainWindowAdmin.cpp) -----
    void build_clients_page();
    void build_resources_page();
    void build_analytics_page();
    void build_reports_page();
    void build_activity_page();
    void build_finance_page();
    void refresh_clients();
    void refresh_resources();
    void refresh_analytics();
    void refresh_activity();
    void refresh_transactions();
    void on_client_deposit_clicked();
    void on_client_discount_clicked();
    void on_new_client();
    void on_add_resource();
    void on_edit_price();
    void on_remove_resource();
    void on_toggle_maintenance();
    void on_report_generate();
    void on_report_save();
    void on_report_open_notepad();
    void on_report_load();
    void on_export_csv();

    // ----- настройки (MainWindowSettings.cpp) -----
    void build_settings_page();
    void on_apply_settings();
    void on_pick_accent();
    void on_reset_accent();
    void on_backup_data();
    void on_open_data_folder();

    // ----- вспомогательное -----
    void run_guarded(const std::function<void()>& action);
    void commit();
    void note(const QString& text);
    int selected_id(QTableWidget* table);
    void set_stat(const std::string& key, const QString& text);
    QString data_path(const QString& file_name) const;
    std::vector<std::shared_ptr<gc::Booking>> visible_bookings() const;
    QString booking_resource_names(const gc::Booking& booking) const;
    bool is_client() const { return user_->get_role() == gc::Role::Client; }
    bool is_owner() const { return user_->get_role() == gc::Role::Owner; }

    gc::GameCenter& center_;
    std::shared_ptr<gc::User> user_;
    std::string data_dir_;
    bool logout_requested_ = false;
    AppSettings settings_;
    QString settings_path_;

    // каркас
    QFrame* sidebar_ = nullptr;
    QVBoxLayout* nav_layout_ = nullptr;
    QStackedWidget* stack_ = nullptr;
    QLabel* logo_label_ = nullptr;
    QLabel* page_title_ = nullptr;
    QLabel* user_label_ = nullptr;
    QLabel* balance_label_ = nullptr;
    std::vector<QPushButton*> nav_buttons_;
    std::vector<QString> page_titles_;
    std::vector<QString> page_icons_;
    int current_page_ = 0;
    QTimer* timer_ = nullptr;
    std::map<std::string, QLabel*> stat_values_;

    // рабочий стол
    ChartWidget* dash_revenue_chart_ = nullptr;
    ChartWidget* dash_hours_chart_ = nullptr;
    QTableWidget* upcoming_table_ = nullptr;

    // карта зала
    QScrollArea* map_scroll_ = nullptr;
    std::map<int, QPushButton*> zone_buttons_;
    std::map<int, QString> zone_state_cache_;

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

    // магазин / склад / профиль
    QListWidget* shop_list_ = nullptr;
    QLabel* shop_info_ = nullptr;
    QTableWidget* stock_table_ = nullptr;
    QProgressBar* profile_progress_ = nullptr;
    QLabel* profile_next_ = nullptr;
    QTableWidget* history_table_ = nullptr;

    // персонал / владелец
    QTableWidget* clients_table_ = nullptr;
    QTableWidget* resources_table_ = nullptr;
    QTableWidget* transactions_table_ = nullptr;
    QLabel* revenue_label_ = nullptr;
    QComboBox* analytics_days_ = nullptr;
    ChartWidget* chart_days_ = nullptr;
    ChartWidget* chart_categories_ = nullptr;
    ChartWidget* chart_resources_ = nullptr;
    ChartWidget* chart_hours_ = nullptr;
    ChartWidget* chart_clients_ = nullptr;
    ChartWidget* chart_products_ = nullptr;
    QPlainTextEdit* report_edit_ = nullptr;
    QPlainTextEdit* activity_edit_ = nullptr;
    QLineEdit* activity_filter_ = nullptr;

    // настройки
    QComboBox* theme_combo_ = nullptr;
    QPushButton* accent_button_ = nullptr;
    QSpinBox* font_spin_ = nullptr;
    QSpinBox* refresh_spin_ = nullptr;
    QLineEdit* club_edit_ = nullptr;
    QString pending_accent_;
};
