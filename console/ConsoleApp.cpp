#include "ConsoleApp.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "Exceptions.h"
#include "Report.h"
#include "Utils.h"

namespace gc {

ConsoleApp::ConsoleApp(GameCenter& center, std::string data_dir, std::istream& in, std::ostream& out)
    : center_(center), data_dir_(std::move(data_dir)), in_(in), out_(out) {}

Minutes ConsoleApp::now() {
    return timeutil::now_minutes();
}

// ------------------------------------------------------------------- ввод

std::string ConsoleApp::read_line(const std::string& prompt) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        throw InputClosed();
    }
    return util::trim(line);
}

int ConsoleApp::read_int(const std::string& prompt) {
    return util::parse_int(read_line(prompt), "число");
}

Money ConsoleApp::read_money(const std::string& prompt) {
    return Money::parse(read_line(prompt));
}

Minutes ConsoleApp::read_datetime(const std::string& prompt) {
    const std::string text = read_line(prompt + " (ГГГГ-ММ-ДД ЧЧ:ММ, Enter = сейчас): ");
    return text.empty() ? now() : timeutil::parse_datetime(text);
}

bool ConsoleApp::confirm(const std::string& prompt) {
    const std::string answer = read_line(prompt + " (y/n): ");
    return answer == "y" || answer == "Y" || answer == "д" || answer == "Д";
}

// ------------------------------------------------------------------- меню

void ConsoleApp::run() {
    out_ << "=== Информационная система игрового центра ===\n";
    try {
        while (running_) {
            if (current_) {
                run_user_menu();
            } else {
                run_guest_menu();
            }
        }
    } catch (const InputClosed&) {
        out_ << "\n";
    }
    center_.save_all(data_dir_);
    out_ << "До свидания!\n";
}

void ConsoleApp::dispatch(const std::vector<MenuItem>& items) {
    out_ << "\n";
    for (const MenuItem& item : items) {
        out_ << " " << item.key << ". " << item.title << "\n";
    }
    try {
        const int choice = read_int("Выбор: ");
        const auto it = std::find_if(items.begin(), items.end(),
                                     [choice](const MenuItem& item) { return item.key == choice; });
        if (it == items.end()) {
            out_ << "Нет такого пункта меню.\n";
            return;
        }
        it->action();
        center_.save_all(data_dir_);
    } catch (const GameCenterException& error) {
        out_ << "Ошибка: " << error.what() << "\n";
    }
}

void ConsoleApp::run_guest_menu() {
    dispatch({{1, "Войти", [this] { login(); }},
              {2, "Зарегистрироваться как клиент", [this] { register_client(); }},
              {0, "Выход", [this] { quit(); }}});
}

void ConsoleApp::run_user_menu() {
    center_.refresh_statuses(now());
    out_ << "\n--- " << current_->full_name() << " (" << to_string(current_->get_role()) << ") ---";
    switch (current_->get_role()) {
        case Role::Client:
            dispatch(client_items());
            break;
        case Role::Administrator:
            dispatch(staff_items(false));
            break;
        case Role::Owner:
            dispatch(staff_items(true));
            break;
    }
}

std::vector<ConsoleApp::MenuItem> ConsoleApp::client_items() {
    return {{1, "Профиль и баланс", [this] { show_profile(); }},
            {2, "Пополнить баланс", [this] { deposit_self(); }},
            {3, "Карта зала (статусы сейчас)", [this] { show_hall_map(); }},
            {4, "Каталог ресурсов", [this] { show_catalog(); }},
            {5, "Начать сеанс сейчас", [this] { start_session(); }},
            {6, "Забронировать ресурс", [this] { book_resource(); }},
            {7, "Мои бронирования", [this] { show_bookings(true); }},
            {8, "Отменить бронь", [this] { cancel_booking(false); }},
            {9, "Конструктор праздника", [this] { plan_event(); }},
            {0, "Выйти из аккаунта", [this] { logout(); }}};
}

std::vector<ConsoleApp::MenuItem> ConsoleApp::staff_items(bool is_owner) {
    std::vector<MenuItem> items = {
        {1, "Карта зала (статусы сейчас)", [this] { show_hall_map(); }},
        {2, "Каталог ресурсов", [this] { show_catalog(); }},
        {3, "Изменить статус ресурса", [this] { change_status(); }},
        {4, "Тех. обслуживание вкл/выкл", [this] { toggle_maintenance(); }},
        {5, "Освободить ресурс (завершить сеанс)", [this] { release_resource(); }},
        {6, "Список клиентов", [this] { show_clients(); }},
        {7, "Пополнить счёт клиента", [this] { deposit_for_client(); }},
        {8, "Задать скидку клиенту", [this] { set_client_discount(); }},
        {9, "Все бронирования", [this] { show_bookings(false); }},
        {10, "Отменить любую бронь", [this] { cancel_booking(true); }},
        {11, "Зарегистрировать клиента", [this] { register_client(); }}};
    if (is_owner) {
        items.push_back({12, "Журнал транзакций и выручка", [this] { show_transactions(); }});
        items.push_back({13, "Добавить ресурс", [this] { add_resource(); }});
        items.push_back({14, "Удалить ресурс", [this] { remove_resource(); }});
        items.push_back({15, "Создать сотрудника", [this] { create_staff(); }});
        items.push_back({16, "Сохранить текстовый отчёт", [this] { save_report(); }});
    }
    items.push_back({0, "Выйти из аккаунта", [this] { logout(); }});
    return items;
}

// ---------------------------------------------------------- общие действия

void ConsoleApp::login() {
    const std::string login_name = read_line("Логин: ");
    const std::string password = read_line("Пароль: ");
    current_ = center_.authenticate(login_name, password);
    out_ << "Добро пожаловать, " << current_->full_name() << "!\n";
}

void ConsoleApp::register_client() {
    const std::string login_name = read_line("Логин: ");
    const std::string password = read_line("Пароль (мин. 4 символа): ");
    const std::string full_name = read_line("ФИО: ");
    const long long birth = timeutil::parse_date_days(read_line("Дата рождения (ГГГГ-ММ-ДД): "));
    const auto client = center_.register_client(login_name, password, full_name, birth);
    out_ << "Клиент зарегистрирован, id = " << client->id() << "\n";
}

void ConsoleApp::logout() {
    current_.reset();
}

void ConsoleApp::quit() {
    running_ = false;
}

void ConsoleApp::show_hall_map() {
    const Minutes current_time = now();
    out_ << "\nКарта зала на " << timeutil::format_datetime(current_time) << "\n"
         << "[ ] свободен  [X] занят  [B] забронирован  [M] обслуживание\n";
    std::string last_kind;
    for (const auto& resource : center_.resources().values()) {
        if (resource->kind() != last_kind) {
            last_kind = resource->kind();
            out_ << "\n" << last_kind << ":\n";
        }
        const ResourceStatus status = center_.get_device_status(resource->id(), current_time);
        const char* mark = " ";
        if (status == ResourceStatus::Occupied) {
            mark = "X";
        } else if (status == ResourceStatus::Booked) {
            mark = "B";
        } else if (status == ResourceStatus::Maintenance) {
            mark = "M";
        }
        out_ << "  [" << mark << "] #" << resource->id() << " " << resource->name() << " - " << to_string(status);
        if (status == ResourceStatus::Occupied) {
            out_ << " до " << timeutil::format_datetime(resource->occupied_until());
        }
        out_ << "\n";
    }
}

void ConsoleApp::show_catalog() {
    out_ << "\n";
    for (const auto& resource : center_.resources().values()) {
        out_ << *resource << "\n";
    }
}

void ConsoleApp::show_bookings(bool only_mine) {
    const int owner_id = only_mine ? current_->id() : -1;
    bool any = false;
    for (const auto& booking : center_.bookings().values()) {
        if (only_mine && booking->client_id() != owner_id) {
            continue;
        }
        any = true;
        out_ << "#" << booking->id() << " " << booking->title() << ", клиент " << booking->client_id() << ", "
             << booking->slot() << ", " << booking->price() << "\n";
        if (const auto event = std::dynamic_pointer_cast<EventBooking>(booking)) {
            print_timeline(event->timeline());
        } else {
            out_ << "    ресурс: " << center_.resources().get(booking->resource_id())->name() << ", участников: "
                 << booking->guests() << "\n";
        }
    }
    if (!any) {
        out_ << "Бронирований нет.\n";
    }
}

void ConsoleApp::cancel_booking(bool as_staff) {
    const int id = read_int("Номер брони: ");
    const Money refund = center_.cancel_booking(id, as_staff ? -1 : current_->id(), now());
    out_ << "Бронь отменена. Возврат: " << refund << "\n";
}

// ------------------------------------------------------------------ клиент

std::shared_ptr<Client> ConsoleApp::current_client() const {
    return center_.get_client(current_->id());
}

void ConsoleApp::show_profile() {
    const auto client = current_client();
    out_ << "Логин: " << client->login() << "\nФИО: " << client->full_name() << "\nДата рождения: "
         << timeutil::format_date(client->birth_days()) << " (возраст "
         << client->age_on(now() / kMinutesPerDay) << ")\nБаланс: " << client->balance()
         << "\nСкидка: " << client->discount_percent() << "%\n";
}

void ConsoleApp::deposit_self() {
    center_.deposit(current_->id(), read_money("Сумма пополнения: "), now());
    out_ << "Баланс: " << current_client()->balance() << "\n";
}

void ConsoleApp::start_session() {
    const int resource_id = read_int("Номер ресурса: ");
    const int minutes = read_int("Длительность (мин): ");
    const int guests = read_int("Количество участников: ");
    const Money cost = center_.start_session(current_->id(), resource_id, minutes, guests, now());
    out_ << "Сеанс начат. Списано: " << cost << ". Баланс: " << current_client()->balance() << "\n";
}

void ConsoleApp::book_resource() {
    const int resource_id = read_int("Номер ресурса: ");
    const Minutes start = read_datetime("Начало");
    const int minutes = read_int("Длительность (мин): ");
    const int guests = read_int("Количество участников: ");
    const auto booking = center_.create_booking(current_->id(), resource_id, TimeSlot(start, minutes), guests, now());
    out_ << "Бронь #" << booking->id() << " создана. Списано: " << booking->price() << "\n";
}

void ConsoleApp::print_timeline(const EventTimeline& timeline) {
    for (const TimelineItem& item : timeline.items) {
        std::vector<std::string> names;
        for (const int id : item.resource_ids) {
            names.push_back(center_.resources().get(id)->name());
        }
        out_ << "    " << timeutil::format_datetime(item.slot.start()) << " (" << item.slot.duration() << " мин) "
             << item.activity << " [" << names.front()
             << (names.size() > 1 ? " и ещё " + std::to_string(names.size() - 1) : "") << "] - " << item.cost << "\n";
    }
    out_ << "    Итого без скидки: " << timeline.base_total << ", скидка " << timeline.discount_percent
         << "%, к оплате: " << timeline.total << "\n";
}

void ConsoleApp::plan_event() {
    EventRequest request;
    request.guests = read_int("Количество гостей: ");
    request.avg_age = read_int("Средний возраст гостей: ");
    request.budget = read_money("Бюджет: ");
    request.start_minute = read_datetime("Начало праздника");
    const std::string stages = read_line("Количество этапов (Enter = 3): ");
    if (!stages.empty()) {
        request.slots_count = util::parse_int(stages, "этапы");
    }

    const EventTimeline timeline = center_.generate_event_timeline(request);
    out_ << "\nПредлагаемая программа:\n";
    print_timeline(timeline);

    if (!confirm("Забронировать мероприятие?")) {
        return;
    }
    const std::string name = read_line("Название мероприятия: ");
    const auto booking =
        center_.create_event_booking(current_->id(), name, timeline, request.guests, request.avg_age, now());
    out_ << "Мероприятие забронировано, бронь #" << booking->id() << "\n";
}

// --------------------------------------------------------------- персонал

void ConsoleApp::change_status() {
    const int id = read_int("Номер ресурса: ");
    const int code = read_int("Статус (0 свободен, 1 занят, 2 забронирован, 3 обслуживание): ");
    if (code < 0 || code > 3) {
        throw InvalidInputException("Неверный код статуса");
    }
    center_.set_device_status(id, static_cast<ResourceStatus>(code));
    out_ << "Статус изменён.\n";
}

void ConsoleApp::toggle_maintenance() {
    const int id = read_int("Номер ресурса: ");
    const bool enable = center_.resources().get(id)->is_available();
    center_.set_zone_maintenance(id, enable);
    out_ << (enable ? "Ресурс переведён на обслуживание.\n" : "Ресурс снова доступен.\n");
}

void ConsoleApp::release_resource() {
    const int id = read_int("Номер ресурса: ");
    center_.resources().get(id)->release();
    out_ << "Ресурс освобождён.\n";
}

void ConsoleApp::show_clients() {
    const Minutes current_time = now();
    for (const auto& user : center_.users().values()) {
        if (const auto client = std::dynamic_pointer_cast<Client>(user)) {
            out_ << "#" << client->id() << " " << client->login() << " - " << client->full_name() << ", возраст "
                 << client->age_on(current_time / kMinutesPerDay) << ", баланс " << client->balance()
                 << ", скидка " << client->discount_percent() << "%\n";
        }
    }
}

void ConsoleApp::deposit_for_client() {
    const int id = read_int("Номер клиента: ");
    center_.deposit(id, read_money("Сумма: "), now());
    out_ << "Баланс клиента: " << center_.get_client(id)->balance() << "\n";
}

void ConsoleApp::set_client_discount() {
    const int id = read_int("Номер клиента: ");
    center_.get_client(id)->set_discount_percent(read_int("Скидка, % (0-50): "));
    out_ << "Скидка установлена.\n";
}

// ---------------------------------------------------------------- владелец

void ConsoleApp::show_transactions() {
    for (const auto& [id, entry] : center_.transactions().entries()) {
        out_ << "#" << id << " " << timeutil::format_datetime(entry->timestamp()) << " клиент "
             << entry->client_id() << " " << to_string(entry->type()) << " " << entry->amount() << " - "
             << entry->description() << "\n";
    }
    out_ << "Выручка (списания - возвраты): " << center_.transactions().total_revenue() << "\n";
}

void ConsoleApp::add_resource() {
    const int type = read_int("Тип (1 ПК, 2 лазертаг, 3 верёвочный городок, 4 скалодром, 5 зона отдыха): ");
    const std::string name = read_line("Название: ");
    const Money price = read_money("Цена (за час / за человека в час): ");
    const int id = center_.resources().next_id();

    if (type == 1) {
        const int vip = read_int("Тариф (0 стандарт, 1 VIP): ");
        center_.add_resource(std::make_shared<PCZone>(id, name, price, vip == 1 ? PcTier::Vip : PcTier::Standard,
                                                      read_line("CPU: "), read_line("GPU: "), read_int("RAM, ГБ: ")));
    } else if (type >= 2 && type <= 4) {
        const int min_age = read_int("Минимальный возраст: ");
        const int max_age = read_int("Максимальный возраст: ");
        const int capacity = read_int("Вместимость: ");
        const int extra = read_int(type == 2 ? "Игроков в команде: " : "Высота, м: ");
        if (type == 2) {
            center_.add_resource(std::make_shared<LaserTagArena>(id, name, price, min_age, max_age, capacity, extra));
        } else if (type == 3) {
            center_.add_resource(std::make_shared<RopePark>(id, name, price, min_age, max_age, capacity, extra));
        } else {
            center_.add_resource(std::make_shared<ClimbingWall>(id, name, price, min_age, max_age, capacity, extra));
        }
    } else if (type == 5) {
        center_.add_resource(std::make_shared<RelaxZone>(id, name, price, read_int("Вместимость: ")));
    } else {
        throw InvalidInputException("Неизвестный тип ресурса");
    }
    out_ << "Ресурс добавлен, id = " << id << "\n";
}

void ConsoleApp::remove_resource() {
    center_.remove_resource(read_int("Номер ресурса: "));
    out_ << "Ресурс удалён.\n";
}

void ConsoleApp::create_staff() {
    const int type = read_int("Роль (1 администратор, 2 владелец): ");
    const std::string login_name = read_line("Логин: ");
    const std::string password = read_line("Пароль: ");
    const std::string full_name = read_line("ФИО: ");
    center_.create_staff(type == 2 ? Role::Owner : Role::Administrator, login_name, password, full_name);
    out_ << "Сотрудник создан.\n";
}

void ConsoleApp::save_report() {
    const std::string path = (std::filesystem::path(data_dir_) / "report.txt").string();
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        throw FileFormatException("Не удалось записать файл " + path);
    }
    file << build_text_report(center_, now());
    out_ << "Отчёт сохранён: " << path << "\n";
}

}  // namespace gc
