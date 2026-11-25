#pragma once

#include <QString>

#include <string>

#include "Theme.h"

// Пользовательские настройки внешнего вида и поведения (хранятся в settings.ini рядом с данными).
struct AppSettings {
    QString theme = "Тёмная";
    QString accent;  // пусто - акцент выбранной темы
    int font_px = 13;
    int refresh_sec = 5;
    QString club_name = "GAME CENTER";

    static AppSettings load(const QString& path);
    void save(const QString& path) const;
};

QString settings_file_path(const std::string& data_dir);

// Палитра темы с учётом пользовательского акцента.
Palette resolve_palette(const AppSettings& settings);

// Применяет тему и размер шрифта ко всему приложению.
void apply_theme(const AppSettings& settings);
