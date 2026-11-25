#pragma once

#include <QString>

#include <vector>

// Цветовая схема приложения. Все цвета - строки вида "#rrggbb".
struct Palette {
    QString name;
    QString bg;        // фон окна
    QString sidebar;   // боковое меню
    QString panel;     // карточки, таблицы, поля ввода
    QString panel2;    // кнопки, заголовки таблиц
    QString hover;     // кнопка при наведении
    QString border;    // рамки
    QString text;      // основной текст
    QString muted;     // второстепенный текст
    QString accent;    // главный акцентный цвет
    QString accent2;   // второй цвет градиентов
    QString selected;  // выделение
};

const std::vector<Palette>& theme_presets();
const Palette* find_theme(const QString& name);

// Текущая палитра (читают самописные виджеты: шкала, графики, иконки).
const Palette& current_palette();
void set_current_palette(const Palette& palette);

// Полная таблица стилей Qt для палитры и размера шрифта.
QString build_stylesheet(const Palette& palette, int font_px);

// Цвета статусов ресурсов одинаковы во всех темах.
QString status_color_hex(int status_index);
