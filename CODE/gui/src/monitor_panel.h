#ifndef MONITOR_PANEL_H
#define MONITOR_PANEL_H

/******************<Summary>*****************
 * Name: monitor_panel.h
 * Purpose:
 *      > Monitoring page: overall status banner, large battery voltage,
 *          battery icon (one bar per cell, colored by the warn / danger /
 *          shutdown thresholds) and a thermometer with the temperature zones
 *      > Read-only, simulation lives on its own page (simulate_panel.h)
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QString>
#include <QWidget>

#include "gauges.h"
#include "status_chips.h"
#include "protocol.h"

class MonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit MonitorPanel(QWidget* parent = nullptr);

signals:
    void unlatchRequested();                // user pressed the latch override button

public slots:
    void onFrame(const proto::BatteryFrame& frame);
    void setConnectionState(bool connected);
    void setReadOnly(bool readOnly);
    void setActiveSettings(const QMap<QString, double>& values);

private:
    QLabel* m_banner;
    QLabel* m_voltage;
    StatusChips* m_chips;
    QPushButton* m_unlatchBtn;
    QLabel* m_info;
    BatteryWidget* m_battery;
    ThermometerWidget* m_thermo;

    Thresholds m_th;
    bool m_connected = false;
    bool m_haveFrame = false;
    bool m_readOnly = false;

    void showNoData(const QString& text);
    QWidget* makeBox(const QString& title, QWidget* content);
    static QString badgeStyle(int level);
};

#endif // MONITOR_PANEL_H
