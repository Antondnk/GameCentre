#include "TimelineView.h"

#include <QColor>
#include <QCursor>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QToolTip>

#include <algorithm>

#include "Icons.h"
#include "Theme.h"
#include "TimeUtils.h"

namespace {

constexpr int kLabelWidth = 170;
constexpr int kHeaderHeight = 30;
constexpr int kRowHeight = 34;

QString qs(const std::string& text) {
    return QString::fromUtf8(text.c_str());
}

QString hhmm(gc::Minutes minutes) {
    return qs(gc::timeutil::format_datetime(minutes).substr(11));
}

QColor segment_color(int kind) {
    switch (kind) {
        case 0:
            return QColor(59, 91, 219);
        case 1:
            return QColor(124, 77, 255);
        case 2:
            return QColor(229, 57, 53);
        default:
            return QColor(110, 116, 140);
    }
}

}  // namespace

TimelineView::TimelineView(const gc::GameCenter& center, QWidget* parent)
    : QWidget(parent), center_(center), day_(gc::timeutil::now_minutes() / gc::kMinutesPerDay) {
    setMouseTracking(true);
    setMinimumWidth(820);
    reload();
}

void TimelineView::set_day(long long day_index) {
    day_ = day_index;
    update();
}

void TimelineView::set_viewer(int user_id, bool anonymize_others) {
    viewer_id_ = user_id;
    anonymize_ = anonymize_others;
}

void TimelineView::reload() {
    const int rows = static_cast<int>(center_.resources().size());
    setMinimumHeight(kHeaderHeight + rows * kRowHeight + 2);
    update();
}

std::vector<TimelineView::Segment> TimelineView::segments_for(int resource_id) const {
    std::vector<Segment> result;
    const gc::Minutes day_start = day_ * gc::kMinutesPerDay;
    const gc::Minutes day_end = day_start + gc::kMinutesPerDay;

    const auto add = [&](int kind, gc::Minutes start, gc::Minutes end, const QString& label,
                         const QString& tooltip) {
        if (end > day_start && start < day_end) {
            result.push_back(Segment{kind, start, end, label, tooltip});
        }
    };

    for (const auto& booking : center_.bookings().values()) {
        const bool hide = anonymize_ && booking->client_id() != viewer_id_;
        const auto owner = center_.users().find(booking->client_id());
        const QString who = hide ? QString("Занято") : (owner ? qs(owner->full_name()) : QString("?"));

        if (const auto event = std::dynamic_pointer_cast<gc::EventBooking>(booking)) {
            for (const gc::TimelineItem& item : event->timeline().items) {
                const auto& ids = item.resource_ids;
                if (std::find(ids.begin(), ids.end(), resource_id) == ids.end()) {
                    continue;
                }
                QString tip = who + "\n" + hhmm(item.slot.start()) + " – " + hhmm(item.slot.end());
                if (!hide) {
                    tip += "\n" + qs(event->title()) + "\n" + qs(item.activity);
                }
                add(1, item.slot.start(), item.slot.end(), who, tip);
            }
        } else if (booking->resource_id() == resource_id) {
            QString tip = who + "\n" + hhmm(booking->slot().start()) + " – " + hhmm(booking->slot().end());
            if (!hide) {
                tip += "\nСтоимость: " + qs(booking->price().to_string());
            }
            add(0, booking->slot().start(), booking->slot().end(), who, tip);
        }
    }

    const auto resource = center_.resources().find(resource_id);
    if (resource) {
        const gc::Minutes now = gc::timeutil::now_minutes();
        if (resource->status() == gc::ResourceStatus::Occupied && resource->occupied_until() > now) {
            add(2, now, resource->occupied_until(), "Сеанс",
                "Идёт сеанс до " + hhmm(resource->occupied_until()));
        } else if (resource->status() == gc::ResourceStatus::Maintenance) {
            add(3, day_start, day_end, "Обслуживание", "Ресурс на техническом обслуживании");
        }
    }
    return result;
}

void TimelineView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const Palette& palette = current_palette();
    painter.fillRect(rect(), QColor(palette.panel));
    hits_.clear();

    const auto resources = center_.resources().values();
    const double hour_width = (width() - kLabelWidth) / 24.0;
    const gc::Minutes day_start = day_ * gc::kMinutesPerDay;
    const gc::Minutes day_end = day_start + gc::kMinutesPerDay;

    // Подписи ресурсов и чередующийся фон строк.
    for (std::size_t i = 0; i < resources.size(); ++i) {
        const int y = kHeaderHeight + static_cast<int>(i) * kRowHeight;
        if (i % 2 == 1) {
            painter.fillRect(QRect(0, y, width(), kRowHeight), QColor(palette.panel2));
        }
        painter.drawPixmap(QRect(10, y + 7, 20, 20),
                           make_icon_pixmap(icon_tag_for(*resources[i]), QColor(palette.muted), 20));
        painter.setPen(QColor(palette.text));
        painter.drawText(QRect(38, y, kLabelWidth - 42, kRowHeight),
                         static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), qs(resources[i]->name()));
    }

    // Вертикальные линии часов и подписи.
    for (int hour = 0; hour <= 24; ++hour) {
        const double x = kLabelWidth + hour * hour_width;
        painter.setPen(QColor(palette.border));
        painter.drawLine(QPointF(x, kHeaderHeight), QPointF(x, height()));
        if (hour < 24) {
            painter.setPen(QColor(palette.muted));
            painter.drawText(QRectF(x, 0, hour_width, kHeaderHeight), static_cast<int>(Qt::AlignCenter),
                             QString::number(hour));
        }
    }

    // Полосы броней.
    for (std::size_t i = 0; i < resources.size(); ++i) {
        const double y = kHeaderHeight + static_cast<double>(i) * kRowHeight;
        for (const Segment& segment : segments_for(resources[i]->id())) {
            const gc::Minutes from = std::max(segment.start, day_start);
            const gc::Minutes to = std::min(segment.end, day_end);
            const double x1 = kLabelWidth + static_cast<double>(from - day_start) / 60.0 * hour_width;
            const double x2 = kLabelWidth + static_cast<double>(to - day_start) / 60.0 * hour_width;
            const QRectF bar(x1, y + 4, std::max(x2 - x1, 4.0), kRowHeight - 8);

            painter.setPen(Qt::NoPen);
            painter.setBrush(segment_color(segment.kind));
            painter.drawRoundedRect(bar, 5, 5);

            painter.setPen(QColor(255, 255, 255));
            const QString text = painter.fontMetrics().elidedText(segment.label, Qt::ElideRight,
                                                                  static_cast<int>(bar.width()) - 10);
            painter.drawText(bar.adjusted(6, 0, -4, 0), static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), text);
            hits_.push_back(Hit{bar, segment.tooltip});
        }
    }

    // Красная линия «сейчас» для сегодняшних суток.
    const gc::Minutes now = gc::timeutil::now_minutes();
    if (now >= day_start && now < day_end) {
        const double x = kLabelWidth + static_cast<double>(now - day_start) / 60.0 * hour_width;
        painter.setPen(QColor(239, 83, 80));
        painter.drawLine(QPointF(x, kHeaderHeight), QPointF(x, height()));
    }
}

void TimelineView::mouseMoveEvent(QMouseEvent*) {
    const QPointF position = mapFromGlobal(QCursor::pos());
    for (const Hit& hit : hits_) {
        if (hit.rect.contains(position)) {
            QToolTip::showText(QCursor::pos(), hit.text, this);
            return;
        }
    }
    QToolTip::hideText();
}
