#include "Theme.h"

// Цвета: фон #11131a, панели #1a1d29, рамки #2b3044, акцент #4c6fff.
QString app_stylesheet() {
    static const char* const kPartOne = R"(
QWidget { background-color: #11131a; color: #e6e8f0; font-size: 13px; }
QLabel { background: transparent; }
QMainWindow, QDialog { background-color: #11131a; }

#sidebar { background-color: #0c0e14; border-right: 1px solid #1f2331; }
#logo { font-size: 18px; font-weight: bold; color: #ffffff; padding: 18px 8px; }
QPushButton#navButton { background: transparent; border: none; border-radius: 8px;
    text-align: left; padding: 11px 14px; color: #a3a9bf; font-size: 14px; }
QPushButton#navButton:hover { background-color: #1a1d29; color: #ffffff; }
QPushButton#navButton:checked { background-color: #222a4d; color: #ffffff; font-weight: bold; }

#pageTitle { font-size: 22px; font-weight: bold; }
#sectionTitle { font-size: 15px; font-weight: bold; color: #c8cce0; padding-top: 6px; }
#chip { background-color: #1a1d29; border: 1px solid #2b3044; border-radius: 14px;
    padding: 6px 12px; color: #c8cce0; }
#muted { color: #8b91a7; }

QPushButton { background-color: #222636; border: 1px solid #2f3550; border-radius: 8px;
    padding: 8px 16px; color: #e6e8f0; }
QPushButton:hover { background-color: #2b3150; }
QPushButton:disabled { color: #5d6277; background-color: #1a1d29; }
QPushButton#accent { background-color: #4c6fff; border: none; color: white; font-weight: bold; }
QPushButton#accent:hover { background-color: #6683ff; }

QFrame#card { background-color: #1a1d29; border: 1px solid #2b3044; border-radius: 12px; }
QLabel#cardCaption { color: #8b91a7; font-size: 12px; }
QLabel#cardValue { font-size: 28px; font-weight: bold; }
)";

    static const char* const kPartTwo = R"(
QLineEdit, QSpinBox, QDoubleSpinBox, QDateTimeEdit, QDateEdit, QComboBox {
    background-color: #1a1d29; border: 1px solid #2f3550; border-radius: 6px;
    padding: 6px 8px; selection-background-color: #4c6fff; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateTimeEdit:focus,
QDateEdit:focus, QComboBox:focus { border: 1px solid #4c6fff; }
QComboBox QAbstractItemView { background-color: #1a1d29; selection-background-color: #2a3566; }

QTableWidget { background-color: #1a1d29; alternate-background-color: #1e2232;
    border: 1px solid #2b3044; border-radius: 8px; gridline-color: #2b3044;
    selection-background-color: #2a3566; selection-color: #ffffff; }
QHeaderView::section { background-color: #222636; color: #8b91a7; border: none;
    border-bottom: 1px solid #2b3044; padding: 8px; font-weight: bold; }

QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: #11131a; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: #2f3550; border-radius: 5px; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: #11131a; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: #2f3550; border-radius: 5px; min-width: 24px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QGroupBox { border: 1px solid #2b3044; border-radius: 10px; margin-top: 14px;
    padding: 12px; font-weight: bold; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; color: #8b91a7; }

QToolTip { background-color: #222636; color: #e6e8f0; border: 1px solid #4c6fff; padding: 6px; }
)";

    return QString::fromUtf8(kPartOne) + QString::fromUtf8(kPartTwo);
}
