#include "plot_widgets.h"

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <limits>

namespace plotdata {

/*******plotdata::info*******
 * Description
        > returns the fixed description (name, units, color, ...) of one series
 * Arguments
        > int series: 0..kSeriesCount-1
 * Returns
        > const SeriesInfo&: description
*/
const SeriesInfo& info(int series){
    static const SeriesInfo table[kSeriesCount] = {
        {"Cell 1",          "V",  QColor("#1f77b4"), 3, 0.02},
        {"Cell 2",          "V",  QColor("#ff7f0e"), 3, 0.02},
        {"Cell 3",          "V",  QColor("#9467bd"), 3, 0.02},
        {"Cell 5",          "V",  QColor("#17becf"), 3, 0.02},
        {"Battery voltage", "V",  QColor("#2ca02c"), 2, 0.10},
        {"Temperature",     "\xC2\xB0""C", QColor("#d62728"), 1, 1.0},
    };
    return table[std::clamp(series, 0, kSeriesCount - 1)];
}

}



/*******PlotData::clear*******
 * Description
        > removes every recorded sample
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void PlotData::clear(){
    t.clear();
    for (auto& s : v){s.clear();}
}



/*******PlotWidget::PlotWidget*******
 * Description
        > creates a plot that reads from shared data
 * Arguments
        > const PlotData* data: recorded samples (owned by the caller, must outlive the widget)
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
PlotWidget::PlotWidget(const PlotData* data, QWidget* parent) : QWidget(parent), m_data(data) {
    setMouseTracking(true);
    setAutoFillBackground(false);
}



/*******PlotWidget::setSeries*******
 * Description
        > chooses which series are drawn (one = absolute axis, several = overlay)
 * Arguments
        > const QList<int>& series: series numbers
 * Returns
        > N/A
*/
void PlotWidget::setSeries(const QList<int>& series){
    m_series = series;
    update();
}



/*******PlotWidget::setWindow*******
 * Description
        > sets how much time the x axis shows
 * Arguments
        > double seconds: window length, 0 = everything recorded
 * Returns
        > N/A
*/
void PlotWidget::setWindow(double seconds){
    m_window = seconds;
    update();
}



/*******PlotWidget::sizeHint*******
 * Description
        > preferred size
 * Arguments
        > N/A
 * Returns
        > QSize: size
*/
QSize PlotWidget::sizeHint() const {return QSize(480, 220);}



/*******PlotWidget::minimumSizeHint*******
 * Description
        > smallest useful size
 * Arguments
        > N/A
 * Returns
        > QSize: size
*/
QSize PlotWidget::minimumSizeHint() const {return QSize(200, 110);}



/*******PlotWidget::formatTime*******
 * Description
        > formats seconds as m:ss or h:mm:ss
 * Arguments
        > double seconds: time
 * Returns
        > QString: text
*/
QString PlotWidget::formatTime(double seconds){
    const long long s = static_cast<long long>(std::llround(std::max(0.0, seconds)));
    if (s >= 3600){return QStringLiteral("%1:%2:%3").arg(s / 3600).arg((s / 60) % 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0'));}
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}



/*******PlotWidget::niceStep*******
 * Description
        > rounds a step up to 1, 2, 5 times a power of ten
 * Arguments
        > double roughStep: wanted step (> 0)
 * Returns
        > double: rounded step
*/
double PlotWidget::niceStep(double roughStep){
    const double p = std::pow(10.0, std::floor(std::log10(roughStep)));
    const double m = roughStep / p;
    return p * (m <= 1.0 ? 1.0 : m <= 2.0 ? 2.0 : m <= 5.0 ? 5.0 : 10.0);
}



/*******PlotWidget::mouseMoveEvent*******
 * Description
        > remembers the mouse x position for the hover readout
 * Arguments
        > QMouseEvent* event: Qt event
 * Returns
        > N/A
*/
void PlotWidget::mouseMoveEvent(QMouseEvent* event){
    m_hoverX = event->position().toPoint().x();
    update();
}



/*******PlotWidget::leaveEvent*******
 * Description
        > removes the hover readout
 * Arguments
        > QEvent* event: Qt event
 * Returns
        > N/A
*/
void PlotWidget::leaveEvent(QEvent* event){
    m_hoverX = -1;
    update();
    QWidget::leaveEvent(event);
}



/*******PlotWidget::paintEvent*******
 * Description
        > draws legend, axes, grid, traces and the hover cursor
 * Arguments
        > QPaintEvent* event: Qt event (unused)
 * Returns
        > N/A
*/
void PlotWidget::paintEvent(QPaintEvent*){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QPalette pal = palette();
    const QColor textColor = pal.color(QPalette::WindowText);
    QColor gridColor = textColor;
    gridColor.setAlpha(45);
    const QFontMetrics fm(font());
    const bool single = (m_series.size() == 1);

    // ---- legend ----
    // one value shown: a single line above the plot. Several values: a column of boxes (one per value) to the right
    // of the plot, each with the name, the current value and the min / max range in a dimmer color.
    struct Item {int series; QString text; int width;};
    QList<Item> items;
    struct Card {int series; QString name; QString value; QString range;};
    QList<Card> cards;
    const int plotLeftProbe = single ? 62 : 12;

    QFont boldFont = font();
    boldFont.setBold(true);
    const QFontMetrics fmBold(boldFont);

    int legendW = 0;                                    // width of the legend column (several values only)
    if (!single){
        int widest = 0;
        for (int s = 0; s < plotdata::kSeriesCount; ++s){
            const auto& inf = plotdata::info(s);
            widest = std::max(widest, fmBold.horizontalAdvance(QString::fromLatin1(inf.name)) + 24 +
                                      fmBold.horizontalAdvance(QStringLiteral("-000.000 %1").arg(QString::fromUtf8(inf.units))));
        }
        widest = std::max(widest, fm.horizontalAdvance(QStringLiteral("min -000.000   max -000.000")) + 8);
        legendW = std::min(widest + 20, std::max(150, width() / 2 - 20));
    }
    const int availW = std::max(50, width() - plotLeftProbe - 10);

    // time under the mouse (needs the x range, computed below), so build the legend after the range
    const int n = m_data->size();
    double x0 = 0.0, x1 = 1.0;
    if (n > 0){
        const double t0 = m_data->t.first();
        const double tEnd = m_data->t.last();
        if (m_window <= 0.0){x0 = t0; x1 = std::max(tEnd, t0 + 1.0);}
        else if (tEnd - t0 < m_window){x0 = t0; x1 = t0 + m_window;}
        else {x0 = tEnd - m_window; x1 = tEnd;}
    }

    // visible index range [i0, i1)
    int i0 = 0, i1 = 0;
    if (n > 0){
        i0 = static_cast<int>(std::lower_bound(m_data->t.begin(), m_data->t.end(), x0) - m_data->t.begin());
        i1 = static_cast<int>(std::upper_bound(m_data->t.begin(), m_data->t.end(), x1) - m_data->t.begin());
        i0 = std::max(0, i0 - 1);
    }

    // per-series visible min / max (NaN gaps skipped) and the value to show in the legend
    const double kNaN = std::numeric_limits<double>::quiet_NaN();
    QVector<double> lo(plotdata::kSeriesCount, kNaN), hi(plotdata::kSeriesCount, kNaN);
    for (int s : m_series){
        for (int i = i0; i < i1; ++i){
            const double v = m_data->v[s][i];
            if (std::isnan(v)){continue;}
            if (std::isnan(lo[s]) || v < lo[s]){lo[s] = v;}
            if (std::isnan(hi[s]) || v > hi[s]){hi[s] = v;}
        }
        if (!std::isnan(lo[s])){
            const double minSpan = plotdata::info(s).minSpan;
            if (hi[s] - lo[s] < minSpan){
                const double mid = 0.5 * (hi[s] + lo[s]);
                lo[s] = mid - 0.5 * minSpan;
                hi[s] = mid + 0.5 * minSpan;
            }
            else {
                const double pad = 0.08 * (hi[s] - lo[s]);
                lo[s] -= pad;
                hi[s] += pad;
            }
        }
    }

    // index of the sample under the mouse
    double hoverT = kNaN;
    int hoverIdx = -1;
    const int L = single ? 62 : 12, R = single ? 12 : legendW + 22, B = fm.height() + 10;
    const int plotW = std::max(10, width() - L - R);
    if (m_hoverX >= L && m_hoverX <= L + plotW && n > 0){
        hoverT = x0 + (m_hoverX - L) * (x1 - x0) / plotW;
        hoverIdx = static_cast<int>(std::lower_bound(m_data->t.begin(), m_data->t.end(), hoverT) - m_data->t.begin());
        if (hoverIdx >= n){hoverIdx = n - 1;}
        if (hoverIdx > 0 && std::abs(m_data->t[hoverIdx - 1] - hoverT) < std::abs(m_data->t[hoverIdx] - hoverT)){hoverIdx--;}
    }

    for (int s : m_series){
        const auto& inf = plotdata::info(s);
        double value = kNaN;
        if (hoverIdx >= 0){value = m_data->v[s][hoverIdx];}
        else {for (int i = n - 1; i >= 0; --i){if (!std::isnan(m_data->v[s][i])){value = m_data->v[s][i]; break;}}}
        const QString valueText = std::isnan(value) ? QStringLiteral("--") : QStringLiteral("%1 %2").arg(value, 0, 'f', inf.decimals).arg(QString::fromUtf8(inf.units));
        QString text = QStringLiteral("%1  %2").arg(QString::fromLatin1(inf.name), valueText);
        if (!single){
            QString range = QStringLiteral("min --   max --");
            // show the real (not padded) range in the overlay so the scaling is readable
            double rl = kNaN, rh = kNaN;
            for (int i = i0; i < i1; ++i){
                const double v = m_data->v[s][i];
                if (std::isnan(v)){continue;}
                if (std::isnan(rl) || v < rl){rl = v;}
                if (std::isnan(rh) || v > rh){rh = v;}
            }
            if (!std::isnan(rl)){range = QStringLiteral("min %1   max %2").arg(rl, 0, 'f', inf.decimals).arg(rh, 0, 'f', inf.decimals);}
            cards.append({s, QString::fromLatin1(inf.name), valueText, range});
        }
        items.append({s, text, fm.horizontalAdvance(text) + 26});
    }

    // flow the legend items into rows
    QList<QList<int>> rows;
    {
        int rowW = 0;
        rows.append(QList<int>());
        for (int k = 0; k < items.size(); ++k){
            if (rowW > 0 && rowW + items[k].width > availW){rows.append(QList<int>()); rowW = 0;}
            rows.last().append(k);
            rowW += items[k].width;
        }
    }
    const int lineH = fm.height() + 4;
    const int T = single ? static_cast<int>(rows.size()) * lineH + 8 : 8;

    // ---- legend: column of boxes to the right of the plot (several values) ----
    if (!single && !cards.isEmpty()){
        const int gap = 6;
        const int x = width() - legendW - 8;
        const int availH = std::max(40, height() - 8 - B);
        const int fullH = 2 * lineH + 10;                       // name + value, and the range below
        const int compactH = lineH + 8;                         // everything on one line pair when space is short
        int cardH = (availH - gap * (static_cast<int>(cards.size()) - 1)) / static_cast<int>(cards.size());
        const bool twoLines = cardH >= fullH - 4;
        cardH = std::min(cardH, fullH + 6);
        cardH = std::max(cardH, compactH);

        QColor dim = textColor;
        dim.setAlpha(255);
        dim = QColor::fromRgbF(0.45 * textColor.redF() + 0.55 * pal.color(QPalette::Window).redF(),
                               0.45 * textColor.greenF() + 0.55 * pal.color(QPalette::Window).greenF(),
                               0.45 * textColor.blueF() + 0.55 * pal.color(QPalette::Window).blueF());

        int y = 4;
        for (const Card& card : cards){
            const QColor c = plotdata::info(card.series).color;
            const QRectF box(x, y, legendW, cardH);

            QColor fill = c;
            fill.setAlpha(28);
            p.setPen(QPen(c, 1.5));
            p.setBrush(fill);
            p.drawRoundedRect(box, 5, 5);

            // colored stripe on the left edge
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawRoundedRect(QRectF(box.left(), box.top(), 6, box.height()), 3, 3);

            const double textLeft = box.left() + 14;
            const double textRight = box.right() - 8;

            p.setFont(boldFont);
            p.setPen(textColor);
            const double line1Y = box.top() + (twoLines ? 4 : (box.height() - fm.height()) / 2.0);
            p.drawText(QRectF(textLeft, line1Y, textRight - textLeft, fm.height()), Qt::AlignLeft | Qt::AlignVCenter, card.name);
            p.drawText(QRectF(textLeft, line1Y, textRight - textLeft, fm.height()), Qt::AlignRight | Qt::AlignVCenter, card.value);

            if (twoLines){
                p.setFont(font());
                p.setPen(dim);
                p.drawText(QRectF(textLeft, line1Y + fm.height() + 1, textRight - textLeft, fm.height()), Qt::AlignLeft | Qt::AlignVCenter, card.range);
            }
            p.setFont(font());

            y += cardH + gap;
        }
    }

    // ---- legend: single line above the plot (one value) ----
    if (single){
        int y = 4;
        for (const auto& row : rows){
            int x = L;
            for (int k : row){
                const QColor c = plotdata::info(items[k].series).color;
                p.setPen(Qt::NoPen);
                p.setBrush(c);
                p.drawRoundedRect(QRectF(x, y + lineH / 2.0 - 4, 14, 8), 2, 2);
                p.setPen(textColor);
                p.drawText(QPointF(x + 19, y + fm.ascent() + 1), items[k].text);
                x += items[k].width;
            }
            y += lineH;
        }
    }

    const QRectF plot(L, T, plotW, std::max(10, height() - T - B));

    // frame
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(gridColor, 1));
    p.drawRect(plot);

    if (n == 0 || m_series.isEmpty()){
        p.setPen(textColor);
        p.drawText(plot, Qt::AlignCenter, m_series.isEmpty() ? tr("No value selected") : tr("No data yet - press \"Start recording\""));
        return;
    }

    // ---- x axis grid + labels ----
    {
        static const double steps[] = {1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600, 7200, 14400, 28800};
        const double want = (x1 - x0) / std::max(2.0, plot.width() / 80.0);
        double step = steps[std::size(steps) - 1];
        for (double s : steps){if (s >= want){step = s; break;}}
        const double first = std::ceil(x0 / step) * step;
        for (double t = first; t <= x1 + 1e-9; t += step){
            const double x = plot.left() + (t - x0) / (x1 - x0) * plot.width();
            p.setPen(QPen(gridColor, 1));
            p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            p.setPen(textColor);
            const double lx = std::clamp(x - 40.0, 0.0, std::max(0.0, width() - 80.0));
            p.drawText(QRectF(lx, plot.bottom() + 3, 80, fm.height()), Qt::AlignHCenter | Qt::AlignTop, formatTime(t));
        }
    }

    // ---- y grid (+ labels when only one series is shown) ----
    if (single){
        const int s = m_series.first();
        if (!std::isnan(lo[s])){
            const double step = niceStep((hi[s] - lo[s]) / std::max(2.0, plot.height() / 40.0));
            const double first = std::ceil(lo[s] / step) * step;
            for (double v = first; v <= hi[s] + 1e-12; v += step){
                const double y = plot.bottom() - (v - lo[s]) / (hi[s] - lo[s]) * plot.height();
                p.setPen(QPen(gridColor, 1));
                p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
                p.setPen(textColor);
                p.drawText(QRectF(0, y - fm.height() / 2.0, L - 6, fm.height()), Qt::AlignRight | Qt::AlignVCenter,
                           QString::number(v, 'f', std::max(0, plotdata::info(s).decimals - (step >= 0.1 ? 1 : 0))));
            }
        }
    }
    else {
        p.setPen(QPen(gridColor, 1));
        for (int k = 1; k < 4; ++k){
            const double y = plot.top() + plot.height() * k / 4.0;
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        }
    }

    // ---- traces ----
    p.save();
    p.setClipRect(plot.adjusted(-1, -1, 1, 1));
    for (int s : m_series){
        if (std::isnan(lo[s])){continue;}
        QPen pen(plotdata::info(s).color, 1.6);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);

        QPolygonF seg;
        auto flush = [&](){
            if (seg.size() == 1){p.drawPoint(seg.first());}
            else if (seg.size() > 1){p.drawPolyline(seg);}
            seg.clear();
        };
        auto toPoint = [&](int i){
            return QPointF(plot.left() + (m_data->t[i] - x0) / (x1 - x0) * plot.width(),
                           plot.bottom() - (m_data->v[s][i] - lo[s]) / (hi[s] - lo[s]) * plot.height());
        };

        // one pixel column keeps its first, lowest, highest and last point, so spikes survive
        int curPx = std::numeric_limits<int>::min();
        int iFirst = -1, iMin = -1, iMax = -1, iLast = -1;
        auto flushColumn = [&](){
            if (iFirst < 0){return;}
            QList<int> idx = {iFirst, iMin, iMax, iLast};
            std::sort(idx.begin(), idx.end());
            idx.erase(std::unique(idx.begin(), idx.end()), idx.end());
            for (int i : idx){seg.append(toPoint(i));}
            iFirst = iMin = iMax = iLast = -1;
        };
        for (int i = i0; i < i1; ++i){
            const double v = m_data->v[s][i];
            if (std::isnan(v)){flushColumn(); flush(); curPx = std::numeric_limits<int>::min(); continue;}
            const int px = static_cast<int>(std::floor(plot.left() + (m_data->t[i] - x0) / (x1 - x0) * plot.width()));
            if (px != curPx){flushColumn(); curPx = px;}
            if (iFirst < 0){iFirst = iMin = iMax = i;}
            if (v < m_data->v[s][iMin]){iMin = i;}
            if (v > m_data->v[s][iMax]){iMax = i;}
            iLast = i;
        }
        flushColumn();
        flush();
    }
    p.restore();

    // ---- hover cursor ----
    if (hoverIdx >= 0){
        const double x = plot.left() + (m_data->t[hoverIdx] - x0) / (x1 - x0) * plot.width();
        p.setPen(QPen(textColor, 1, Qt::DashLine));
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        for (int s : m_series){
            const double v = m_data->v[s][hoverIdx];
            if (std::isnan(v) || std::isnan(lo[s])){continue;}
            const double y = plot.bottom() - (v - lo[s]) / (hi[s] - lo[s]) * plot.height();
            p.setPen(Qt::NoPen);
            p.setBrush(plotdata::info(s).color);
            p.drawEllipse(QPointF(x, y), 3.5, 3.5);
        }
        const QString tt = formatTime(m_data->t[hoverIdx]);
        const QRectF tipRect(x - fm.horizontalAdvance(tt) / 2.0 - 4, plot.top() + 2, fm.horizontalAdvance(tt) + 8, fm.height() + 2);
        QRectF clamped = tipRect;
        if (clamped.left() < plot.left()){clamped.moveLeft(plot.left());}
        if (clamped.right() > plot.right()){clamped.moveRight(plot.right());}
        p.setPen(Qt::NoPen);
        p.setBrush(pal.color(QPalette::ToolTipBase));
        p.drawRoundedRect(clamped, 3, 3);
        p.setPen(pal.color(QPalette::ToolTipText));
        p.drawText(clamped, Qt::AlignCenter, tt);
    }
}
