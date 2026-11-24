#ifndef GAUGES_H
#define GAUGES_H

/******************<Summary>*****************
 * Name: gauges.h
 * Purpose:
 *      > Custom-painted widgets for the Monitor page:
 *          BatteryWidget      - battery icon with one fill bar per populated cell
 *          ThermometerWidget  - thermometer with colored temperature zones
 *      > Colors follow the RP2350's thresholds (good / warning / danger / shutdown)
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QColor>
#include <QWidget>

#include "protocol.h"

struct Thresholds {
    double cellWarn = 3.8;          // used until the real settings are read from the RP2350
    double cellDanger = 3.69;
    double cellShutdown = 3.65;
    double tempWarn = 45.0;
    double tempDanger = 50.0;
    double tempShutdown = 60.0;
};

QColor levelColor(int level);

// Scale (relative to the base font) of the cell-voltage text and of every box title.
// Box titles are slightly larger than the cell voltages.
constexpr double kCellVoltScale = 1.7;
constexpr double kBoxTitleScale = 2.0;
int levelForLow(double value, double warn, double danger, double shutdown);
int levelForHigh(double value, double warn, double danger, double shutdown);



class BatteryWidget : public QWidget {
    Q_OBJECT

public:
    explicit BatteryWidget(QWidget* parent = nullptr);

    void setThresholds(const Thresholds& t);
    void setCells(const double* volts, unsigned ovrMask, bool hasData);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Thresholds m_th;
    double m_volts[proto::kNumCells] = {0, 0, 0, 0, 0};
    unsigned m_ovr = 0;
    bool m_hasData = false;

    static constexpr double kFullCellVolts = 4.2;       // 100 % on the icon
};



class ThermometerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ThermometerWidget(QWidget* parent = nullptr);

    void setThresholds(const Thresholds& t);
    void setTemperature(double celsius, bool simulated, bool hasData);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Thresholds m_th;
    double m_temp = 0.0;
    bool m_simulated = false;
    bool m_hasData = false;
};

#endif // GAUGES_H
