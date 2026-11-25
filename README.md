# Информационная система игрового центра

Курсовой проект по дисциплине «Программирование на языках высокого уровня» (БГУИР).
Тема: **информационная система игрового центра** — киберспортивные ПК-зоны и площадки
для активных игр (лазертаг, верёвочный городок, скалодром).

> Текущий этап: расширенная бета. Ядро (`gamecenter_core`) + консольный интерфейс + простой графический интерфейс
> на Qt Widgets. Оба интерфейса работают с одним и тем же ядром и форматом данных.

## Возможности

- Роли: клиент, администратор, владелец (разные меню и права).
- Учёт ресурсов: ПК (Standard/VIP), лазертаг, верёвочный городок, скалодром, зона отдыха; статусы
  «Свободен / Занят / Забронирован / На обслуживании», карта зала.
- Возрастные ограничения зон и ночное ограничение (23:00–06:00) для несовершеннолетних.
- Баланс клиента, пополнение, списание (исключение при нехватке средств), журнал транзакций.
- Сеансы «прямо сейчас», бронирование на интервал с проверкой пересечений, отмена с возвратом
  (≥ 24 ч — 100 %, ≥ 2 ч — 50 %, иначе 0 %).
- **Конструктор мероприятий (киллер-фича)**: по числу гостей, среднему возрасту и бюджету подбирает
  таймлайн праздника (например, лазертаг → ПК-турнир → отдых), проверяет возраст, вместимость и
  занятость, применяет пакетную скидку и бронирует все ресурсы сразу.
- Магазин и бар (товары, склад, покупки с учётом скидки), программа лояльности (Новичок → … по сумме трат).
- Дополнительные зоны: VR-арена, игровые консоли, бильярд; ресурсы и товары добавляются из интерфейса.
- Аналитика: выручка по дням и направлениям, загрузка ресурсов, популярные часы, топ клиентов и товаров.
- Отчёты: текстовый отчёт (сохранить/загрузить/открыть в Блокноте), экспорт транзакций в CSV, журнал действий.
- Настройки GUI: темы, свой акцентный цвет, размер шрифта, название клуба, резервная копия данных (`settings.ini`).
- Хранение данных в текстовых файлах (`fstream`), пароли — SHA-256 с солью.

## Сборка и запуск

Требуется компилятор с поддержкой C++17 и CMake ≥ 3.16.

```bash
cmake -S . -B build
cmake --build build
./build/gamecenter_console          # данные хранятся в папке ./data
./build/gamecenter_console my_data  # или в указанной папке
ctest --test-dir build              # автотесты
```

### Графический интерфейс (Qt)

Нужен Qt 6 (подойдёт и Qt 5) с модулем Widgets. Если Qt не найден, CMake просто пропустит GUI.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="путь/к/Qt/6.x/компилятор"   # например C:/Qt/6.7.2/msvc2019_64
cmake --build build
./build/gamecenter_gui
```

Боковое меню с иконками, 6 тем оформления. Страницы клиента: **рабочий стол**, **карта зала**,
**бронирование**, **конструктор праздника**, **магазин и бар**, **мой профиль** (лояльность, история), **настройки**.
Персонал: **клиенты**, **ресурсы** (добавление зон без консоли), **склад и товары**, **журнал действий**.
Владелец дополнительно: **аналитика** (графики), **финансы** (+CSV), **отчёты** (Блокнот, сохранить/загрузить).

При первом запуске создаются демо-данные. Тестовые учётные записи:

| Роль | Логин | Пароль |
|------|-------|--------|
| Владелец | `owner` | `owner123` |
| Администратор | `admin` | `admin123` |
| Клиент (взрослый) | `client` | `client123` |
| Клиент (ребёнок, 12 лет) | `kid` | `kid12345` |

Формат даты и времени при вводе: `ГГГГ-ММ-ДД ЧЧ:ММ`.

## Структура проекта

```
include/ , src/     ядро (логика без интерфейса)
  Money, TimeSlot, TimeUtils, Utils, Sha256   — вспомогательные типы
  Resource (PCZone, ActiveZone, LaserTagArena, RopePark, ClimbingWall, RelaxZone, VRZone, ConsoleZone, BilliardsTable)
  Product, Order, Analytics, Report
  User (Client, Administrator, Owner)
  Booking (EventBooking), Timeline, Transaction
  Repository<T>                                — шаблонный контейнер + файлы
  GameCenter                                   — фасад бизнес-логики
  EventPlanner                                 — конструктор мероприятий
console/            консольный интерфейс (ConsoleApp)
gui/                графический интерфейс Qt (MainWindow*.cpp, Dialogs, TimelineView, ChartWidget, Icons, Theme, AppSettings)
main.cpp            точка входа
tests/              автотесты
```

## Использованные возможности C++

Классы и объекты; дружественные функции и перегрузка операций (`Money`, `TimeSlot`, `Resource`);
наследование и абстрактные классы (`Resource`, `User`, `Booking`); виртуальные функции; шаблоны
классов и функций (`Repository<T>`, `join_values`, `split_values`); исключения (иерархия
`GameCenterException`); потоки и файлы; контейнеры и итераторы STL (`vector`, `map`, `set`, алгоритмы).

## Соответствие функциям из пояснительной записки

| В записке | В коде |
|-----------|--------|
| authenticate, get_role | `GameCenter::authenticate`, `User::get_role` |
| deposit, withdraw | `GameCenter::deposit`, `GameCenter::withdraw` (+ `Client::add_funds/charge`) |
| get_device_status, set_device_status | `GameCenter::get_device_status`, `GameCenter::set_device_status` |
| calculate_pc_cost, get_pc_specs | `PCZone::calculate_cost`, `PCZone::get_pc_specs` |
| check_age_validity, validate_zone_age | `GameCenter::check_age_validity`, `GameCenter::validate_zone_age` |
| set_zone_maintenance | `GameCenter::set_zone_maintenance` |
| create_booking, cancel_booking, create_event_booking | одноимённые методы `GameCenter` |
| generate_event_timeline, check_timeline_intersections, calculate_package_discount | методы `EventPlanner` |
| save_to_file, load_from_file | `Repository<T>::save_to_file/load_from_file`, `GameCenter::save_all/load_all` |
| add_item, remove_item | `Repository<T>::add_item/remove_item` |
| on_zone_clicked, on_start_session_clicked, refresh_hall_map | `MainWindow::on_zone_clicked` (начало сеанса — в его диалоге), `MainWindow::refresh_hall_map` (по `QTimer`) |

## SonarCloud

Ссылка: _будет добавлена после подключения репозитория_.
