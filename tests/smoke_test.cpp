#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "EventPlanner.h"
#include "Exceptions.h"
#include "GameCenter.h"
#include "Sha256.h"

namespace {

int failures = 0;

void check(bool condition, const char* description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << "\n";
        ++failures;
    }
}

template <typename Exception, typename Action>
bool throws(Action action) {
    try {
        action();
    } catch (const Exception&) {
        return true;
    }
    return false;
}

}  // namespace

int main() {
    using namespace gc;

    check(sha256_hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256");
    check(Money::parse("12,5").cents() == 1250, "money parse");

    const Minutes now = timeutil::parse_datetime("2026-11-09 12:00");
    GameCenter center;
    center.seed_demo_data(now);

    const auto adult = center.authenticate("client", "client123");
    const auto kid = center.authenticate("kid", "kid12345");
    check(throws<AuthenticationException>([&] { center.authenticate("client", "bad"); }), "wrong password");

    // Возраст: ребёнок не может бронировать ночью и в зону с ограничением по возрасту.
    const TimeSlot night(timeutil::parse_datetime("2026-11-10 23:30"), 60);
    check(throws<AgeValidationException>([&] { center.create_booking(kid->id(), 1, night, 1, now); }),
          "minor at night");
    const TimeSlot day(timeutil::parse_datetime("2026-11-10 15:00"), 60);

    // Недостаток средств.
    check(throws<InsufficientFundsException>([&] { center.withdraw(kid->id(), Money::from_double(1000), "x", now); }),
          "insufficient funds");

    // Конфликт броней и отмена с возвратом.
    const auto booking = center.create_booking(adult->id(), 1, day, 1, now);
    check(throws<BookingConflictException>([&] { center.create_booking(kid->id(), 1, day, 1, now); }), "conflict");
    check(center.cancel_booking(booking->id(), adult->id(), now) == booking->price(), "full refund");

    // Конструктор мероприятий + бронирование.
    EventRequest request;
    request.guests = 8;
    request.avg_age = 12;
    request.budget = Money::from_double(400);
    request.start_minute = timeutil::parse_datetime("2026-11-12 14:00");
    const EventTimeline timeline = center.generate_event_timeline(request);
    check(timeline.items.size() == 3, "timeline size");
    check(timeline.total <= request.budget, "timeline budget");
    center.deposit(adult->id(), Money::from_double(500), now);
    center.create_event_booking(adult->id(), "ДР", timeline, request.guests, request.avg_age, now);
    check(!EventPlanner(center).check_timeline_intersections(timeline).empty(), "event blocks resources");
    check(throws<AgeValidationException>(
              [&] { center.create_event_booking(kid->id(), "x", timeline, 8, 12, now); }),
          "minor organizer");

    // Сохранение и загрузка.
    const std::string dir = (std::filesystem::temp_directory_path() / "gc_smoke").string();
    center.save_all(dir);
    GameCenter loaded;
    check(loaded.load_all(dir), "load");
    check(loaded.resources().size() == center.resources().size(), "resources persisted");
    check(loaded.bookings().size() == center.bookings().size(), "bookings persisted");
    check(loaded.authenticate("owner", "owner123")->get_role() == Role::Owner, "login after load");
    std::filesystem::remove_all(dir);

    if (failures == 0) {
        std::cout << "All smoke tests passed\n";
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
