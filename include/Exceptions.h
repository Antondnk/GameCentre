#pragma once

#include <stdexcept>
#include <string>

#include "Money.h"

namespace gc {

// Базовое исключение предметной области.
class GameCenterException : public std::runtime_error {
public:
    explicit GameCenterException(const std::string& message) : std::runtime_error(message) {}
};

// Недостаточно средств на балансе клиента.
class InsufficientFundsException : public GameCenterException {
public:
    InsufficientFundsException(const Money& required, const Money& available)
        : GameCenterException("Недостаточно средств: требуется " + required.to_string() +
                              ", на балансе " + available.to_string()) {}
};

// Возраст клиента не подходит под правила зоны или клуба.
class AgeValidationException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

// Пересечение тайм-слотов бронирования.
class BookingConflictException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

// Ресурс недоступен (обслуживание, занят).
class ResourceUnavailableException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class DuplicateItemException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class ItemNotFoundException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class AuthenticationException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class AccessDeniedException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class InvalidInputException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class FileFormatException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

class EventPlanningException : public GameCenterException {
public:
    using GameCenterException::GameCenterException;
};

}  // namespace gc
