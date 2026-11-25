#include "ChartWidget.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>

#include <algorithm>

#include "Theme.h"

namespace {

constexpr double kTitleHeight = 34.0;
constexpr double kPadding = 14.0;

QString value_label(const ChartWidget::Bar& bar) {
    return bar.value_text.isEmpty() ? QString::number(bar.value, 'f', 2) : bar.value_text;
}

double max_value(const std::vector<ChartWidget::Bar>& bars) {
    double maximum = 0.0;
    for (const auto& bar : bars) {
        maximum = std::max(maximum, bar.value);
    }
    return maximum > 0.0 ? maximum : 1.0;
}

QBrush bar_brush(double x1, double y1, double x2, double y2) {
    QLinearGradient gradient(x1, y1, x2, y2);
    gradient.setColorAt(0.0, QColor(current_palette().accent));
    gradient.setColorAt(1.0, QColor(current_palette().accent2));
    return QBrush(gradient);
}

}  // namespace

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(230);
}

void ChartWidget::set_title(const QString& title) {
    title_ = title;
    update();
}

void ChartWidget::set_orientation(Orientation orientation) {
    orientation_ = orientation;
    update();
}

void ChartWidget::set_data(std::vector<Bar> bars) {
    bars_ = std::move(bars);
    update();
}

void ChartWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const Palette& palette = current_palette();

    // Фон-карточка.
    painter.setPen(QPen(QColor(palette.border), 1));
    painter.setBrush(QBrush(QColor(palette.panel)));
    painter.drawRoundedRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0), 12, 12);

    // Заголовок.
    QFont title_font = painter.font();
    title_font.setBold(true);
    painter.setFont(title_font);
    painter.setPen(QColor(palette.text));
    painter.drawText(QRectF(kPadding, 6, width() - 2 * kPadding, kTitleHeight - 6),
                     static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), title_);
    QFont normal_font = title_font;
    normal_font.setBold(false);
    painter.setFont(normal_font);

    const QRectF area(kPadding, kTitleHeight + 4, width() - 2 * kPadding, height() - kTitleHeight - 4 - kPadding);
    if (bars_.empty()) {
        painter.setPen(QColor(palette.muted));
        painter.drawText(area, static_cast<int>(Qt::AlignCenter), "Нет данных");
        return;
    }
    if (orientation_ == Orientation::Vertical) {
        paint_vertical(painter, area);
    } else {
        paint_horizontal(painter, area);
    }
}

void ChartWidget::paint_vertical(QPainter& painter, const QRectF& area) {
    const Palette& palette = current_palette();
    const double label_height = 20.0;
    const double value_height = 16.0;
    const double plot_height = area.height() - label_height - value_height;
    const double slot = area.width() / static_cast<double>(bars_.size());
    const double bar_width = std::min(slot * 0.62, 56.0);
    const double maximum = max_value(bars_);
    const double baseline = area.top() + value_height + plot_height;

    painter.setPen(QPen(QColor(palette.border), 1));
    painter.drawLine(QPointF(area.left(), baseline), QPointF(area.right(), baseline));

    for (std::size_t i = 0; i < bars_.size(); ++i) {
        const Bar& bar = bars_[i];
        const double center = area.left() + slot * (static_cast<double>(i) + 0.5);
        const double height = std::max(bar.value / maximum * plot_height, bar.value > 0.0 ? 3.0 : 0.0);
        const QRectF rect(center - bar_width / 2.0, baseline - height, bar_width, height);

        if (height > 0.0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(bar_brush(rect.left(), rect.top(), rect.left(), rect.bottom()));
            painter.drawRoundedRect(rect, 4, 4);
        }
        painter.setPen(QColor(palette.muted));
        if (slot >= 34.0 || i % 2 == 0) {
            painter.drawText(QRectF(center - slot / 2.0, rect.top() - value_height, slot, value_height),
                             static_cast<int>(Qt::AlignCenter), value_label(bar));
        }
        painter.drawText(QRectF(center - slot / 2.0, baseline + 2, slot, label_height),
                         static_cast<int>(Qt::AlignCenter), bar.label);
    }
}

void ChartWidget::paint_horizontal(QPainter& painter, const QRectF& area) {
    const Palette& palette = current_palette();
    const double label_width = std::min(150.0, area.width() * 0.38);
    const double value_width = 80.0;
    const double plot_width = area.width() - label_width - value_width;
    const double row = std::min(30.0, area.height() / static_cast<double>(bars_.size()));
    const double bar_height = std::max(row * 0.6, 6.0);
    const double maximum = max_value(bars_);

    for (std::size_t i = 0; i < bars_.size(); ++i) {
        const Bar& bar = bars_[i];
        const double top = area.top() + row * static_cast<double>(i);
        const QFontMetrics metrics = painter.fontMetrics();

        painter.setPen(QColor(palette.text));
        painter.drawText(QRectF(area.left(), top, label_width - 8, row), static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft),
                         metrics.elidedText(bar.label, Qt::ElideRight, static_cast<int>(label_width) - 8));

        const double width = std::max(bar.value / maximum * plot_width, bar.value > 0.0 ? 3.0 : 0.0);
        const QRectF rect(area.left() + label_width, top + (row - bar_height) / 2.0, width, bar_height);
        if (width > 0.0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(bar_brush(rect.left(), rect.top(), rect.right(), rect.top()));
            painter.drawRoundedRect(rect, 4, 4);
        }
        painter.setPen(QColor(palette.muted));
        painter.drawText(QRectF(rect.right() + 6, top, value_width, row), static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft),
                         value_label(bar));
    }
}
