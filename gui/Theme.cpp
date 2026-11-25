#include "Theme.h"

namespace {

Palette g_current = {"Тёмная", "#11131a", "#0c0e14", "#1a1d29", "#222636", "#2b3150",
                     "#2b3044", "#e6e8f0", "#8b91a7", "#4c6fff", "#7c4dff", "#2a3566"};

// Шаблон стилей: токены @bg@, @panel@ и т.д. заменяются цветами палитры.
const char* const kPartOne = R"(
QWidget { background-color: @bg@; color: @text@; font-size: @font@px; }
QLabel { background: transparent; }
QMainWindow, QDialog { background-color: @bg@; }

#sidebar { background-color: @sidebar@; border-right: 1px solid @border@; }
#logo { font-size: 18px; font-weight: bold; color: @accent@; padding: 16px 8px; }
QPushButton#navButton { background: transparent; border: none; border-radius: 8px;
    text-align: left; padding: 9px 12px; color: @muted@; font-size: @font@px; }
QPushButton#navButton:hover { background-color: @panel@; color: @text@; }
QPushButton#navButton:checked { background-color: @selected@; color: @text@; font-weight: bold; }

#pageTitle { font-size: 22px; font-weight: bold; }
#sectionTitle { font-size: 15px; font-weight: bold; padding-top: 6px; }
#chip { background-color: @panel@; border: 1px solid @border@; border-radius: 14px; padding: 6px 12px; }
#muted { color: @muted@; }

QPushButton { background-color: @panel2@; border: 1px solid @border@; border-radius: 8px;
    padding: 8px 16px; color: @text@; }
QPushButton:hover { background-color: @hover@; }
QPushButton:disabled { color: @muted@; background-color: @panel@; }
QPushButton#accent { background-color: @accent@; border: none; color: #ffffff; font-weight: bold; }
QPushButton#accent:hover { background-color: @accent2@; }
QPushButton#danger { background-color: #c0392b; border: none; color: #ffffff; }
QPushButton#danger:hover { background-color: #e74c3c; }

QFrame#card { background-color: @panel@; border: 1px solid @border@; border-radius: 12px; }
QLabel#cardCaption { color: @muted@; font-size: 12px; }
QLabel#cardValue { font-size: 26px; font-weight: bold; }
)";

const char* const kPartTwo = R"(
QLineEdit, QSpinBox, QDoubleSpinBox, QDateTimeEdit, QDateEdit, QComboBox {
    background-color: @panel@; border: 1px solid @border@; border-radius: 6px;
    padding: 6px 8px; selection-background-color: @accent@; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateTimeEdit:focus,
QDateEdit:focus, QComboBox:focus { border: 1px solid @accent@; }
QComboBox QAbstractItemView { background-color: @panel@; selection-background-color: @selected@; }

QPlainTextEdit { background-color: @panel@; border: 1px solid @border@; border-radius: 8px;
    padding: 8px; selection-background-color: @accent@; }

QTableWidget { background-color: @panel@; alternate-background-color: @panel2@;
    border: 1px solid @border@; border-radius: 8px; gridline-color: @border@;
    selection-background-color: @selected@; selection-color: @text@; }
QHeaderView::section { background-color: @panel2@; color: @muted@; border: none;
    border-bottom: 1px solid @border@; padding: 8px; font-weight: bold; }

QListWidget { background-color: @panel@; border: 1px solid @border@; border-radius: 8px; padding: 6px; }
QListWidget::item { background-color: @panel2@; border: 1px solid @border@; border-radius: 10px;
    padding: 8px; margin: 4px; }
QListWidget::item:hover { background-color: @hover@; }
QListWidget::item:selected { background-color: @selected@; border: 1px solid @accent@; color: @text@; }

QProgressBar { background-color: @panel2@; border: 1px solid @border@; border-radius: 7px;
    text-align: center; height: 14px; color: @text@; }
QProgressBar::chunk { background-color: @accent@; border-radius: 6px; }
)";

const char* const kPartThree = R"(
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: @bg@; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: @border@; border-radius: 5px; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: @bg@; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: @border@; border-radius: 5px; min-width: 24px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QGroupBox { border: 1px solid @border@; border-radius: 10px; margin-top: 14px;
    padding: 12px; font-weight: bold; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: @muted@; }

QToolTip { background-color: @panel2@; color: @text@; border: 1px solid @accent@; padding: 6px; }
QMessageBox QLabel { color: @text@; }
)";

}  // namespace

const std::vector<Palette>& theme_presets() {
    static const std::vector<Palette> presets = {
        {"Тёмная", "#11131a", "#0c0e14", "#1a1d29", "#222636", "#2b3150", "#2b3044", "#e6e8f0", "#8b91a7",
         "#4c6fff", "#7c4dff", "#2a3566"},
        {"Светлая", "#f3f5fa", "#ffffff", "#ffffff", "#eef1f8", "#e1e6f5", "#d5dbea", "#1d2233", "#6b7390",
         "#3b5bdb", "#7c4dff", "#cfd9ff"},
        {"Фиолетовая", "#1b1030", "#2b1055", "#2a1a4a", "#35215c", "#45296f", "#4a3380", "#f1eaff", "#b7a6d9",
         "#a855f7", "#ec4899", "#5b3aa0"},
        {"Неон", "#0a0a12", "#07070d", "#12121f", "#1a1a2e", "#24243d", "#2a2a4a", "#e8f9ff", "#7f8fa6",
         "#00e5ff", "#ff2bd6", "#0b3d4a"},
    };
    return presets;
}

const Palette* find_theme(const QString& name) {
    for (const Palette& palette : theme_presets()) {
        if (palette.name == name) {
            return &palette;
        }
    }
    return nullptr;
}

const Palette& current_palette() {
    return g_current;
}

void set_current_palette(const Palette& palette) {
    g_current = palette;
}

QString build_stylesheet(const Palette& palette, int font_px) {
    QString sheet = QString::fromUtf8(kPartOne) + QString::fromUtf8(kPartTwo) + QString::fromUtf8(kPartThree);
    sheet.replace("@bg@", palette.bg);
    sheet.replace("@sidebar@", palette.sidebar);
    sheet.replace("@panel2@", palette.panel2);
    sheet.replace("@panel@", palette.panel);
    sheet.replace("@hover@", palette.hover);
    sheet.replace("@border@", palette.border);
    sheet.replace("@text@", palette.text);
    sheet.replace("@muted@", palette.muted);
    sheet.replace("@accent2@", palette.accent2);
    sheet.replace("@accent@", palette.accent);
    sheet.replace("@selected@", palette.selected);
    sheet.replace("@font@", QString::number(font_px));
    return sheet;
}

QString status_color_hex(int status_index) {
    switch (status_index) {
        case 0:
            return "#2ecc71";  // свободен
        case 1:
            return "#ef5350";  // занят
        case 2:
            return "#fbc02d";  // забронирован
        default:
            return "#7b8094";  // обслуживание
    }
}
