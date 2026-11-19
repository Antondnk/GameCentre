#pragma once

#include <QRectF>
#include <QString>
#include <QWidget>

#include <vector>

#include "GameCenter.h"

// Шкала бронирований на выбранные сутки: строка на каждый ресурс, по горизонтали - часы.
class TimelineView : public QWidget {
public:
    explicit TimelineView(const gc::GameCenter& center, QWidget* parent = nullptr);

    // Сутки задаются числом дней от 1970-01-01.
    void set_day(long long day_index);
    // Для клиента чужие брони показываются обезличенно ("Занято").
    void set_viewer(int user_id, bool anonymize_others);
    // Пересчитывает высоту и перерисовывает шкалу.
    void reload();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    struct Segment {
        int kind;  // 0 - бронь, 1 - мероприятие, 2 - текущий сеанс, 3 - обслуживание
        gc::Minutes start;
        gc::Minutes end;
        QString label;
        QString tooltip;
    };
    struct Hit {
        QRectF rect;
        QString text;
    };

    std::vector<Segment> segments_for(int resource_id) const;

    const gc::GameCenter& center_;
    long long day_;
    int viewer_id_ = -1;
    bool anonymize_ = false;
    std::vector<Hit> hits_;
};
