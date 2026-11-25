#pragma once

// Общие вспомогательные функции GUI (все inline, чтобы не плодить cpp-файлы).

#include <QByteArray>
#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

#include <string>

#include "Icons.h"
#include "Money.h"
#include "TimeUtils.h"

inline QString qs(const std::string& text) {
    return QString::fromUtf8(text.c_str());
}

inline QString money_text(const gc::Money& money) {
    return qs(money.to_string());
}

// QDateTime -> минуты ядра (локальное время без часовых поясов).
inline gc::Minutes to_minutes(const QDateTime& value) {
    const QDate date = value.date();
    const QTime time = value.time();
    return gc::timeutil::days_from_civil(date.year(), static_cast<unsigned>(date.month()),
                                         static_cast<unsigned>(date.day())) *
               gc::kMinutesPerDay +
           time.hour() * 60 + time.minute();
}

inline long long to_day_index(const QDate& date) {
    return gc::timeutil::days_from_civil(date.year(), static_cast<unsigned>(date.month()),
                                         static_cast<unsigned>(date.day()));
}

inline void set_cell(QTableWidget* table, int row, int column, const QString& text) {
    table->setItem(row, column, new QTableWidgetItem(text));
}

inline QTableWidget* make_table(QWidget* parent, const QStringList& headers) {
    auto* table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    return table;
}

inline QLabel* make_section_title(const QString& text, QWidget* parent) {
    auto* label = new QLabel(text, parent);
    label->setObjectName("sectionTitle");
    return label;
}

// Карточка-счётчик: подпись, крупное значение и иконка справа.
inline QFrame* make_card(QWidget* parent, const QString& caption, const QString& color, const QString& icon_tag,
                         QLabel** value_out) {
    auto* card = new QFrame(parent);
    card->setObjectName("card");
    auto* row = new QHBoxLayout(card);
    auto* texts = new QVBoxLayout();

    auto* caption_label = new QLabel(caption, card);
    caption_label->setObjectName("cardCaption");
    auto* value_label = new QLabel("0", card);
    value_label->setObjectName("cardValue");
    value_label->setStyleSheet("color: " + color + ";");
    texts->addWidget(caption_label);
    texts->addWidget(value_label);
    row->addLayout(texts, 1);

    auto* icon_label = new QLabel(card);
    icon_label->setPixmap(make_icon_pixmap(icon_tag, QColor(color), 34));
    icon_label->setFixedSize(40, 40);
    icon_label->setScaledContents(true);
    row->addWidget(icon_label);

    if (value_out != nullptr) {
        *value_out = value_label;
    }
    return card;
}

// Запись текста в UTF-8 с BOM (чтобы Блокнот Windows сразу распознал кодировку).
inline bool write_text_file(const QString& path, const QString& text) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QByteArray("\xEF\xBB\xBF"));
    file.write(text.toUtf8());
    return true;
}

// Чтение текстового файла в UTF-8 (BOM отбрасывается).
inline bool read_text_file(const QString& path, QString* text) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray bytes = file.readAll();
    if (bytes.startsWith("\xEF\xBB\xBF")) {
        bytes.remove(0, 3);
    }
    *text = QString::fromUtf8(bytes);
    return true;
}
