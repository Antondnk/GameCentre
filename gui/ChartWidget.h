#pragma once

#include <QRectF>
#include <QString>
#include <QWidget>

#include <vector>

class QPainter;

// Простая столбчатая диаграмма (вертикальная или горизонтальная), рисуется через QPainter.
class ChartWidget : public QWidget {
public:
    struct Bar {
        QString label;
        double value = 0.0;
        QString value_text;  // подпись значения; если пусто - число с двумя знаками
    };
    enum class Orientation { Vertical, Horizontal };

    explicit ChartWidget(QWidget* parent = nullptr);

    void set_title(const QString& title);
    void set_orientation(Orientation orientation);
    void set_data(std::vector<Bar> bars);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void paint_vertical(QPainter& painter, const QRectF& area);
    void paint_horizontal(QPainter& painter, const QRectF& area);

    QString title_;
    Orientation orientation_ = Orientation::Vertical;
    std::vector<Bar> bars_;
};
