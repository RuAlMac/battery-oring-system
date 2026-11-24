#include "gauges.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

/*******levelColor*******
 * Description
        > gets the display color for a battery level
 * Arguments
        > int level: 0=good 1=warning 2=danger 3=shutdown, anything else = no data
 * Returns
        > QColor: green / yellow / orange / red / gray
*/
QColor levelColor(int level){
    switch (level){
        case 0:     {return QColor(0x2e, 0x9e, 0x4a);}
        case 1:     {return QColor(0xf9, 0xc4, 0x25);}
        case 2:     {return QColor(0xf0, 0x7a, 0x1a);}
        case 3:     {return QColor(0xd3, 0x2f, 0x2f);}
        default:    {return QColor(0x75, 0x75, 0x75);}
    }
}



/*******levelForLow*******
 * Description
        > level for a value where LOW is bad (voltages); same rule as the firmware
 * Arguments
        > double value: reading
        > double warn: warning threshold
        > double danger: danger threshold
        > double shutdown: shutdown threshold
 * Returns
        > int: 0=good 1=warning 2=danger 3=shutdown
*/
int levelForLow(double value, double warn, double danger, double shutdown){
    if (value <= shutdown){return 3;}
    if (value <= danger){return 2;}
    if (value <= warn){return 1;}
    return 0;
}



/*******levelForHigh*******
 * Description
        > level for a value where HIGH is bad (temperature); same rule as the firmware
 * Arguments
        > double value: reading
        > double warn: warning threshold
        > double danger: danger threshold
        > double shutdown: shutdown threshold
 * Returns
        > int: 0=good 1=warning 2=danger 3=shutdown
*/
int levelForHigh(double value, double warn, double danger, double shutdown){
    if (value >= shutdown){return 3;}
    if (value >= danger){return 2;}
    if (value >= warn){return 1;}
    return 0;
}



/*******BatteryWidget::BatteryWidget*******
 * Description
        > creates the battery icon widget
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
BatteryWidget::BatteryWidget(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}



/*******BatteryWidget::setThresholds*******
 * Description
        > sets the voltage thresholds used for fill colors and percentages
 * Arguments
        > const Thresholds& t: thresholds from the RP2350's active settings
 * Returns
        > N/A
*/
void BatteryWidget::setThresholds(const Thresholds& t){
    m_th = t;
    update();
}



/*******BatteryWidget::setCells*******
 * Description
        > sets the cell voltages to draw
 * Arguments
        > const double* volts: kNumCells voltages (the unpopulated cell is skipped when drawing)
        > unsigned ovrMask: override mask, bits 0-4 mark simulated cells
        > bool hasData: false = draw empty / gray
 * Returns
        > N/A
*/
void BatteryWidget::setCells(const double* volts, unsigned ovrMask, bool hasData){
    for (int i = 0; i < proto::kNumCells; i++){m_volts[i] = volts[i];}
    m_ovr = ovrMask;
    m_hasData = hasData;
    update();
}



/*******BatteryWidget::sizeHint*******
 * Description
        > preferred size of the icon
 * Arguments
        > N/A
 * Returns
        > QSize: preferred size
*/
QSize BatteryWidget::sizeHint() const {
    return QSize(520, 250);
}



/*******BatteryWidget::minimumSizeHint*******
 * Description
        > smallest useful size of the icon
 * Arguments
        > N/A
 * Returns
        > QSize: minimum size
*/
QSize BatteryWidget::minimumSizeHint() const {
    return QSize(400, 200);
}



/*******BatteryWidget::paintEvent*******
 * Description
        > draws the battery body, terminal nub, and one fill bar per populated cell
            with its voltage, percentage, and color
 * Arguments
        > QPaintEvent* event: unused
 * Returns
        > N/A
*/
void BatteryWidget::paintEvent(QPaintEvent* event){
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor text = palette().color(QPalette::WindowText);
    const QColor outline = palette().color(QPalette::Mid).darker(130);
    const QColor trough = palette().color(QPalette::AlternateBase);

    // populated cells in order
        int cellSlots[proto::kNumCells];
        int n = 0;
        for (int i = 0; i < proto::kNumCells; i++){
            if (i != proto::kUnpopulatedCell){cellSlots[n++] = i;}
        }

    // geometry
        const double nubW = 16.0;
        QRectF body(14.0, 14.0, width() - 14.0 * 2 - nubW, height() - 28.0);
        QRectF nub(body.right(), body.center().y() - body.height() * 0.15, nubW, body.height() * 0.30);

        p.setPen(QPen(outline, 4.0));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(body, 14.0, 14.0);
        p.setPen(Qt::NoPen);
        p.setBrush(outline);
        p.drawRoundedRect(nub, 5.0, 5.0);

    // cells
        const double pad = 14.0;
        const double gap = 12.0;
        QRectF inner = body.adjusted(pad, pad, -pad, -pad);
        double cellW = (inner.width() - gap * (n - 1)) / n;

        QFont nameFont = font();
        nameFont.setPointSizeF(font().pointSizeF() * 0.95);
        QFont voltFont = font();
        voltFont.setBold(true);
        voltFont.setPointSizeF(font().pointSizeF() * kCellVoltScale);
        QFont pctFont = font();
        pctFont.setPointSizeF(font().pointSizeF() * 0.95);

        const double nameH = QFontMetrics(nameFont).height() + 4;
        const double voltH = QFontMetrics(voltFont).height() + 2;
        const double pctH = QFontMetrics(pctFont).height() + 2;

        for (int k = 0; k < n; k++){
            int cell = cellSlots[k];
            double v = m_volts[cell];
            double x = inner.left() + k * (cellW + gap);

            QRectF nameRect(x, inner.top(), cellW, nameH);
            QRectF barRect(x, inner.top() + nameH, cellW, inner.height() - nameH - voltH - pctH);
            QRectF voltRect(x, barRect.bottom() + 2, cellW, voltH);
            QRectF pctRect(x, voltRect.bottom(), cellW, pctH);

            bool simulated = (m_ovr & (1u << cell)) != 0;

            // name
                p.setFont(nameFont);
                p.setPen(text);
                p.drawText(nameRect, Qt::AlignCenter, simulated ? tr("Cell %1  (SIM)").arg(cell + 1) : tr("Cell %1").arg(cell + 1));

            // trough
                p.setPen(Qt::NoPen);
                p.setBrush(trough);
                p.drawRoundedRect(barRect, 8.0, 8.0);

            // fill
                if (m_hasData){
                    int level = levelForLow(v, m_th.cellWarn, m_th.cellDanger, m_th.cellShutdown);
                    double span = std::max(0.05, kFullCellVolts - m_th.cellShutdown);
                    double frac = std::min(1.0, std::max(0.0, (v - m_th.cellShutdown) / span));
                    double minFrac = 0.04;      // always show a sliver so an empty cell is still visible
                    double h = barRect.height() * std::max(frac, minFrac);

                    QRectF fill(barRect.left(), barRect.bottom() - h, barRect.width(), h);

                    QPainterPath clip;
                    clip.addRoundedRect(barRect, 8.0, 8.0);
                    p.save();
                    p.setClipPath(clip);
                    p.setBrush(levelColor(level));
                    p.drawRect(fill);
                    p.restore();

                    // threshold ticks (warn / danger) across the bar
                    p.setPen(QPen(QColor(0, 0, 0, 90), 1.5, Qt::DashLine));
                    for (double th : {m_th.cellWarn, m_th.cellDanger}){
                        double fy = barRect.bottom() - barRect.height() * std::min(1.0, std::max(0.0, (th - m_th.cellShutdown) / span));
                        p.drawLine(QPointF(barRect.left() + 2, fy), QPointF(barRect.right() - 2, fy));
                    }
                }

            // outline (purple when simulated)
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(simulated ? QColor(0x8e, 0x24, 0xaa) : outline, simulated ? 3.5 : 1.5));
                p.drawRoundedRect(barRect, 8.0, 8.0);

            // voltage + percent
                p.setPen(text);
                p.setFont(voltFont);
                p.drawText(voltRect, Qt::AlignCenter, m_hasData ? QStringLiteral("%1 V").arg(v, 0, 'f', 3) : QStringLiteral("- V"));

                p.setFont(pctFont);
                if (m_hasData){
                    double span = std::max(0.05, kFullCellVolts - m_th.cellShutdown);
                    int pct = (int)std::lround(100.0 * std::min(1.0, std::max(0.0, (v - m_th.cellShutdown) / span)));
                    p.drawText(pctRect, Qt::AlignCenter, QStringLiteral("%1 %").arg(pct));
                }
        }
}



/*******ThermometerWidget::ThermometerWidget*******
 * Description
        > creates the thermometer widget
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
ThermometerWidget::ThermometerWidget(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}



/*******ThermometerWidget::setThresholds*******
 * Description
        > sets the temperature thresholds used for the zones and fill color
 * Arguments
        > const Thresholds& t: thresholds from the RP2350's active settings
 * Returns
        > N/A
*/
void ThermometerWidget::setThresholds(const Thresholds& t){
    m_th = t;
    update();
}



/*******ThermometerWidget::setTemperature*******
 * Description
        > sets the temperature to draw
 * Arguments
        > double celsius: temperature the system is using
        > bool simulated: true if it is an override
        > bool hasData: false = draw empty / gray
 * Returns
        > N/A
*/
void ThermometerWidget::setTemperature(double celsius, bool simulated, bool hasData){
    m_temp = celsius;
    m_simulated = simulated;
    m_hasData = hasData;
    update();
}



/*******ThermometerWidget::sizeHint*******
 * Description
        > preferred size of the thermometer
 * Arguments
        > N/A
 * Returns
        > QSize: preferred size
*/
QSize ThermometerWidget::sizeHint() const {
    return QSize(275, 250);
}



/*******ThermometerWidget::minimumSizeHint*******
 * Description
        > smallest useful size of the thermometer
 * Arguments
        > N/A
 * Returns
        > QSize: minimum size
*/
QSize ThermometerWidget::minimumSizeHint() const {
    return QSize(265, 200);
}



/*******ThermometerWidget::paintEvent*******
 * Description
        > draws the tube and bulb filled to the current temperature (bulb always
            completely filled), the colored zones (good / warning / danger /
            shutdown) well clear of the bulb, the threshold labels, and the
            temperature centered under the bulb
 * Arguments
        > QPaintEvent* event: unused
 * Returns
        > N/A
*/
void ThermometerWidget::paintEvent(QPaintEvent* event){
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QColor text = palette().color(QPalette::WindowText);
    const QColor outline = palette().color(QPalette::Mid).darker(130);
    const QColor trough = palette().color(QPalette::AlternateBase);

    // scale
        const double lo = -20.0;
        const double hi = std::max(m_th.tempShutdown + 15.0, 60.0);
        auto clampT = [&](double t){return std::min(hi, std::max(lo, t));};

    // geometry
        QFont bigFont = font();
        bigFont.setBold(true);
        bigFont.setPointSizeF(font().pointSizeF() * 1.6);
        const double textH = QFontMetrics(bigFont).height() + 4;

        const double bulbR = 24.0;
        const double tubeW = 22.0;
        const double top = 12.0;
        const double bulbCy = height() - textH - bulbR - 8.0;
        const double tubeBottom = bulbCy;
        QString value = m_hasData ? QStringLiteral("%1 \u00B0C").arg(m_temp, 0, 'f', 1) : QStringLiteral("- \u00B0C");
        if (m_simulated){value += tr(" (SIM)");}
        const double valueHalfW = QFontMetrics(bigFont).horizontalAdvance(value) / 2.0 + 6.0;

        // center line shared by tube, bulb, and the value text under them
        const double cx = std::max(valueHalfW, std::max(54.0, std::min(width() * 0.40, width() - 175.0)));
        QRectF tube(cx - tubeW / 2.0, top, tubeW, tubeBottom - top);
        QPointF bulbC(cx, bulbCy);

        auto yFor = [&](double t){
            double f = (clampT(t) - lo) / (hi - lo);
            return tube.bottom() - f * tube.height();
        };

    // outline + trough (tube and bulb as one shape)
        QPainterPath tubePath;
        tubePath.addRoundedRect(tube, tubeW / 2, tubeW / 2);
        QPainterPath bulbPath;
        bulbPath.addEllipse(bulbC, bulbR, bulbR);
        QPainterPath shapeOutline = tubePath.united(bulbPath);

        p.setPen(Qt::NoPen);
        p.setBrush(trough);
        p.drawPath(shapeOutline);

    // fill: from the temperature level down through the whole bulb
        int level = m_hasData ? levelForHigh(m_temp, m_th.tempWarn, m_th.tempDanger, m_th.tempShutdown) : -1;
        if (m_hasData){
            double fillTop = std::min(yFor(m_temp), tube.bottom());
            QPainterPath fillRegion;
            fillRegion.addRect(QRectF(bulbC.x() - bulbR - 2, fillTop, 2 * bulbR + 4, (bulbCy + bulbR + 2) - fillTop));
            p.setBrush(levelColor(level));
            p.drawPath(fillRegion.intersected(shapeOutline));
        }

        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(m_simulated ? QColor(0x8e, 0x24, 0xaa) : outline, m_simulated ? 4.0 : 2.5));
        p.drawPath(shapeOutline);

    // zone bar, well to the right of the bulb
        const double zoneX = bulbC.x() + bulbR + 16.0;
        const double zoneW = 10.0;
        struct Zone {double from; double to; int lvl;};
        const Zone zones[] = {
            {lo, m_th.tempWarn, 0},
            {m_th.tempWarn, m_th.tempDanger, 1},
            {m_th.tempDanger, m_th.tempShutdown, 2},
            {m_th.tempShutdown, hi, 3}};

        p.setPen(Qt::NoPen);
        for (const Zone& z : zones){
            double y1 = yFor(z.to);
            double y2 = yFor(z.from);
            if (y2 - y1 < 0.5){continue;}
            p.setBrush(levelColor(z.lvl));
            p.drawRect(QRectF(zoneX, y1, zoneW, y2 - y1));
        }

    // threshold labels + ticks
        QFont small = font();
        small.setPointSizeF(font().pointSizeF() * 0.95);
        p.setFont(small);
        const double labelX = zoneX + zoneW + 6.0;
        const QFontMetrics fm(small);

        struct Tick {double t; QString name;};
        const Tick ticks[] = {
            {m_th.tempWarn, tr("warn")},
            {m_th.tempDanger, tr("danger")},
            {m_th.tempShutdown, tr("shutdown")}};

        p.setPen(text);
        for (const Tick& t : ticks){
            double y = yFor(t.t);
            p.drawLine(QPointF(zoneX - 3, y), QPointF(zoneX + zoneW + 3, y));
            p.drawText(QPointF(labelX, y + fm.ascent() / 2.0 - 1), QStringLiteral("%1 \u00B0C %2").arg(t.t, 0, 'f', 0).arg(t.name));
        }

    // current-value marker (left of the tube)
        if (m_hasData){
            double y = yFor(m_temp);
            QPainterPath arrow;
            arrow.moveTo(tube.left() - 4, y);
            arrow.lineTo(tube.left() - 16, y - 7);
            arrow.lineTo(tube.left() - 16, y + 7);
            arrow.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(text);
            p.drawPath(arrow);
        }

    // value, centered directly under the bulb
        p.setFont(bigFont);
        p.setPen(text);
        p.drawText(QRectF(cx - valueHalfW, height() - textH, 2.0 * valueHalfW, textH), Qt::AlignCenter, value);
}
