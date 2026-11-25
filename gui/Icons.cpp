#include "Icons.h"

#include <QBrush>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr int kCanvas = 64;  // все иконки рисуются в координатах 64x64

QPointF pt(double x, double y) {
    return QPointF(x, y);
}

void line(QPainter& p, double x1, double y1, double x2, double y2) {
    p.drawLine(pt(x1, y1), pt(x2, y2));
}

void circle(QPainter& p, double x, double y, double r) {
    p.drawEllipse(pt(x, y), r, r);
}

void draw_pc(QPainter& p, const QColor& c, bool vip) {
    p.drawRoundedRect(QRectF(8, 10, 48, 32), 4, 4);
    line(p, 32, 42, 32, 52);
    line(p, 20, 54, 44, 54);
    if (vip) {
        QPolygonF diamond;
        diamond << pt(32, 17) << pt(40, 26) << pt(32, 35) << pt(24, 26);
        p.setBrush(c);
        p.drawPolygon(diamond);
    }
}

void draw_laser(QPainter& p) {
    circle(p, 32, 32, 20);
    circle(p, 32, 32, 7);
    line(p, 32, 6, 32, 18);
    line(p, 32, 46, 32, 58);
    line(p, 6, 32, 18, 32);
    line(p, 46, 32, 58, 32);
}

void draw_rope(QPainter& p) {
    line(p, 12, 10, 12, 58);
    line(p, 52, 10, 52, 58);
    QPainterPath upper;
    upper.moveTo(12, 20);
    upper.quadTo(32, 44, 52, 20);
    p.drawPath(upper);
    QPainterPath lower;
    lower.moveTo(12, 36);
    lower.quadTo(32, 60, 52, 36);
    p.drawPath(lower);
}

void draw_wall(QPainter& p, const QColor& c) {
    p.drawRoundedRect(QRectF(10, 8, 44, 48), 4, 4);
    p.setBrush(c);
    circle(p, 22, 20, 4);
    circle(p, 40, 28, 4);
    circle(p, 24, 38, 4);
    circle(p, 42, 46, 4);
}

void draw_relax(QPainter& p) {
    p.drawRoundedRect(QRectF(12, 26, 30, 28), 6, 6);
    p.drawArc(QRectF(36, 30, 16, 16), -90 * 16, 180 * 16);
    line(p, 21, 8, 21, 18);
    line(p, 31, 8, 31, 18);
}

void draw_vr(QPainter& p) {
    p.drawRoundedRect(QRectF(6, 20, 52, 28), 11, 11);
    circle(p, 22, 34, 6);
    circle(p, 42, 34, 6);
    line(p, 32, 42, 32, 48);
}

void draw_console(QPainter& p, const QColor& c) {
    p.drawRoundedRect(QRectF(6, 20, 52, 28), 13, 13);
    line(p, 19, 28, 19, 42);
    line(p, 12, 35, 26, 35);
    p.setBrush(c);
    circle(p, 43, 31, 3);
    circle(p, 49, 37, 3);
    circle(p, 37, 37, 3);
}

void draw_billiard(QPainter& p, const QColor& c) {
    circle(p, 32, 32, 22);
    p.setBrush(c);
    circle(p, 32, 32, 9);
    p.setBrush(Qt::NoBrush);
    p.drawArc(QRectF(14, 14, 20, 20), 100 * 16, 80 * 16);
}

void draw_cart(QPainter& p, const QColor& c) {
    QPolygonF cart;
    cart << pt(6, 12) << pt(16, 12) << pt(24, 40) << pt(50, 40) << pt(56, 18) << pt(18, 18);
    p.drawPolyline(cart);
    p.setBrush(c);
    circle(p, 28, 50, 4);
    circle(p, 46, 50, 4);
}

void draw_dash(QPainter& p) {
    p.drawRoundedRect(QRectF(10, 10, 20, 20), 4, 4);
    p.drawRoundedRect(QRectF(34, 10, 20, 20), 4, 4);
    p.drawRoundedRect(QRectF(10, 34, 20, 20), 4, 4);
    p.drawRoundedRect(QRectF(34, 34, 20, 20), 4, 4);
}

void draw_map(QPainter& p, const QColor& c) {
    circle(p, 32, 26, 14);
    line(p, 22, 37, 32, 58);
    line(p, 42, 37, 32, 58);
    p.setBrush(c);
    circle(p, 32, 26, 4);
}

void draw_calendar(QPainter& p, const QColor& c) {
    p.drawRoundedRect(QRectF(10, 14, 44, 40), 5, 5);
    line(p, 10, 26, 54, 26);
    line(p, 22, 8, 22, 18);
    line(p, 42, 8, 42, 18);
    p.setBrush(c);
    circle(p, 22, 38, 2.5);
    circle(p, 32, 38, 2.5);
    circle(p, 42, 38, 2.5);
}

void draw_chart(QPainter& p, const QColor& c) {
    p.setBrush(c);
    p.drawRoundedRect(QRectF(12, 34, 10, 20), 2, 2);
    p.drawRoundedRect(QRectF(27, 22, 10, 32), 2, 2);
    p.drawRoundedRect(QRectF(42, 10, 10, 44), 2, 2);
}

void draw_report(QPainter& p) {
    p.drawRoundedRect(QRectF(14, 8, 36, 48), 4, 4);
    line(p, 22, 22, 42, 22);
    line(p, 22, 32, 42, 32);
    line(p, 22, 42, 36, 42);
}

void draw_list(QPainter& p, const QColor& c) {
    p.setBrush(c);
    for (int i = 0; i < 3; ++i) {
        const double y = 16 + i * 16;
        circle(p, 14, y, 3);
        line(p, 24, y, 52, y);
    }
}

void draw_settings(QPainter& p) {
    circle(p, 32, 32, 13);
    circle(p, 32, 32, 5);
    for (int i = 0; i < 8; ++i) {
        const double angle = i * kPi / 4.0;
        line(p, 32 + std::cos(angle) * 17, 32 + std::sin(angle) * 17, 32 + std::cos(angle) * 25,
             32 + std::sin(angle) * 25);
    }
}

void draw_clients(QPainter& p) {
    circle(p, 32, 22, 9);
    p.drawArc(QRectF(14, 38, 36, 36), 0, 180 * 16);
}

void draw_coins(QPainter& p) {
    circle(p, 32, 32, 22);
    circle(p, 32, 32, 14);
    line(p, 32, 24, 32, 40);
}

void draw_gift(QPainter& p) {
    p.drawRoundedRect(QRectF(12, 28, 40, 26), 3, 3);
    p.drawRoundedRect(QRectF(9, 19, 46, 9), 3, 3);
    line(p, 32, 19, 32, 54);
    circle(p, 24, 13, 6);
    circle(p, 40, 13, 6);
}

void draw_box(QPainter& p) {
    p.drawRoundedRect(QRectF(10, 16, 44, 38), 4, 4);
    line(p, 10, 28, 54, 28);
    line(p, 32, 28, 32, 40);
}

void draw_drink(QPainter& p) {
    p.drawRoundedRect(QRectF(22, 24, 20, 32), 5, 5);
    p.drawRoundedRect(QRectF(27, 8, 10, 16), 3, 3);
    line(p, 22, 38, 42, 38);
}

void draw_food(QPainter& p, const QColor& c) {
    QPolygonF slice;
    slice << pt(10, 14) << pt(54, 14) << pt(32, 56);
    p.drawPolygon(slice);
    p.setBrush(c);
    circle(p, 25, 24, 3.5);
    circle(p, 39, 24, 3.5);
    circle(p, 32, 38, 3.5);
}

void draw_mouse(QPainter& p) {
    p.drawRoundedRect(QRectF(20, 8, 24, 48), 12, 12);
    line(p, 32, 8, 32, 28);
    line(p, 20, 28, 44, 28);
}

void draw_star(QPainter& p, const QColor& c) {
    QPolygonF star;
    for (int i = 0; i < 10; ++i) {
        const double radius = (i % 2 == 0) ? 24.0 : 10.0;
        const double angle = -kPi / 2.0 + i * kPi / 5.0;
        star << pt(32 + std::cos(angle) * radius, 33 + std::sin(angle) * radius);
    }
    p.setBrush(c);
    p.drawPolygon(star);
}

void draw_default(QPainter& p) {
    circle(p, 32, 32, 20);
}

void draw_tag(QPainter& p, const QString& tag, const QColor& color) {
    if (tag == "PC") {
        draw_pc(p, color, false);
    } else if (tag == "VIP") {
        draw_pc(p, color, true);
    } else if (tag == "LASER") {
        draw_laser(p);
    } else if (tag == "ROPE") {
        draw_rope(p);
    } else if (tag == "WALL") {
        draw_wall(p, color);
    } else if (tag == "RELAX") {
        draw_relax(p);
    } else if (tag == "VR") {
        draw_vr(p);
    } else if (tag == "CONSOLE") {
        draw_console(p, color);
    } else if (tag == "BILLIARD") {
        draw_billiard(p, color);
    } else if (tag == "CART") {
        draw_cart(p, color);
    } else if (tag == "DASH") {
        draw_dash(p);
    } else if (tag == "MAP") {
        draw_map(p, color);
    } else if (tag == "CALENDAR") {
        draw_calendar(p, color);
    } else if (tag == "CHART") {
        draw_chart(p, color);
    } else if (tag == "REPORT") {
        draw_report(p);
    } else if (tag == "LIST") {
        draw_list(p, color);
    } else if (tag == "SETTINGS") {
        draw_settings(p);
    } else if (tag == "CLIENTS") {
        draw_clients(p);
    } else if (tag == "COINS") {
        draw_coins(p);
    } else if (tag == "GIFT") {
        draw_gift(p);
    } else if (tag == "BOX") {
        draw_box(p);
    } else if (tag == "DRINK") {
        draw_drink(p);
    } else if (tag == "FOOD") {
        draw_food(p, color);
    } else if (tag == "MOUSE") {
        draw_mouse(p);
    } else if (tag == "STAR") {
        draw_star(p, color);
    } else {
        draw_default(p);
    }
}

}  // namespace

QPixmap make_icon_pixmap(const QString& tag, const QColor& color, int size) {
    const int pixels = size * 2;  // рисуем в двойном размере для чёткости на HiDPI
    QPixmap pixmap(pixels, pixels);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(static_cast<double>(pixels) / kCanvas, static_cast<double>(pixels) / kCanvas);
    painter.setPen(QPen(color, 3.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    draw_tag(painter, tag, color);
    painter.end();
    return pixmap;
}

QIcon make_icon(const QString& tag, const QColor& color, int size) {
    // Без статического кэша: статические QPixmap могут падать при выходе после QApplication.
    return QIcon(make_icon_pixmap(tag, color, size));
}

QString icon_tag_for(const gc::Resource& resource) {
    if (resource.is_pc()) {
        const auto* pc = dynamic_cast<const gc::PCZone*>(&resource);
        if (pc != nullptr && pc->tier() == gc::PcTier::Vip) {
            return "VIP";
        }
    }
    return QString::fromStdString(resource.type_tag());
}

QString icon_tag_for_category(const std::string& category) {
    if (category == "Напитки") {
        return "DRINK";
    }
    if (category == "Еда") {
        return "FOOD";
    }
    if (category == "Аксессуары") {
        return "MOUSE";
    }
    return "GIFT";
}
