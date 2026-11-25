#include "AppSettings.h"

#include <QApplication>
#include <QDir>
#include <QSettings>

AppSettings AppSettings::load(const QString& path) {
    AppSettings settings;
    QSettings file(path, QSettings::IniFormat);
    settings.theme = file.value("theme", settings.theme).toString();
    settings.accent = file.value("accent", settings.accent).toString();
    settings.font_px = file.value("font_px", settings.font_px).toInt();
    settings.refresh_sec = file.value("refresh_sec", settings.refresh_sec).toInt();
    settings.club_name = file.value("club_name", settings.club_name).toString();

    if (settings.font_px < 10 || settings.font_px > 20) {
        settings.font_px = 13;
    }
    if (settings.refresh_sec < 2 || settings.refresh_sec > 60) {
        settings.refresh_sec = 5;
    }
    if (settings.club_name.trimmed().isEmpty()) {
        settings.club_name = "GAME CENTER";
    }
    return settings;
}

void AppSettings::save(const QString& path) const {
    QSettings file(path, QSettings::IniFormat);
    file.setValue("theme", theme);
    file.setValue("accent", accent);
    file.setValue("font_px", font_px);
    file.setValue("refresh_sec", refresh_sec);
    file.setValue("club_name", club_name);
    file.sync();
}

QString settings_file_path(const std::string& data_dir) {
    return QDir(QString::fromStdString(data_dir)).absoluteFilePath("settings.ini");
}

Palette resolve_palette(const AppSettings& settings) {
    const Palette* preset = find_theme(settings.theme);
    Palette palette = preset ? *preset : theme_presets().front();
    if (!settings.accent.isEmpty()) {
        palette.accent = settings.accent;
    }
    return palette;
}

void apply_theme(const AppSettings& settings) {
    const Palette palette = resolve_palette(settings);
    set_current_palette(palette);
    qApp->setStyleSheet(build_stylesheet(palette, settings.font_px));
}
