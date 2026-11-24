#ifndef PLOT_WIDGETS_H
#define PLOT_WIDGETS_H

/******************<Summary>*****************
 * Name: plot_widgets.h
 * Purpose:
 *      > PlotData: the recorded time series (shared by every plot of the Plots page)
 *      > PlotWidget: custom-painted line plot
 *          - one series  -> own value axis with tick labels
 *          - several     -> overlay, every trace scaled to its own min / max
 *                           (legend shows the real value and range of each trace)
 *      > Hovering the mouse over a plot shows the values at that time
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QColor>
#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

namespace plotdata {

constexpr int kSeriesCount = 6;     // cell 1, cell 2, cell 3, cell 5, battery, temperature

struct SeriesInfo {
    const char* name;
    const char* units;
    QColor color;
    int decimals;
    double minSpan;                 // smallest value range that is still drawn zoomed in
};

const SeriesInfo& info(int series);

}

struct PlotData {
    QVector<double> t;                              // seconds since the first recorded sample
    QVector<double> v[plotdata::kSeriesCount];      // NaN = gap in the recording

    void clear();
    int size() const {return t.size();}
};



class PlotWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlotWidget(const PlotData* data, QWidget* parent = nullptr);

    void setSeries(const QList<int>& series);
    void setWindow(double seconds);                 // 0 = show everything
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    const PlotData* m_data;
    QList<int> m_series;
    double m_window = 0.0;
    int m_hoverX = -1;

    static QString formatTime(double seconds);
    static double niceStep(double roughStep);
};

#endif // PLOT_WIDGETS_H
