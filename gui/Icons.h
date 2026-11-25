#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

#include <string>

#include "Resource.h"

// Векторные иконки, нарисованные кодом (без внешних файлов): перекрашиваются под тему и статус.
// Теги: PC VIP LASER ROPE WALL RELAX VR CONSOLE BILLIARD
//       DASH MAP CALENDAR CHART REPORT SETTINGS CLIENTS COINS GIFT BOX LIST
//       CART DRINK FOOD MOUSE STAR
QPixmap make_icon_pixmap(const QString& tag, const QColor& color, int size);
QIcon make_icon(const QString& tag, const QColor& color, int size = 32);

// Иконка для ресурса (VIP-компьютеры отличаются от обычных).
QString icon_tag_for(const gc::Resource& resource);
// Иконка для категории товара магазина.
QString icon_tag_for_category(const std::string& category);
