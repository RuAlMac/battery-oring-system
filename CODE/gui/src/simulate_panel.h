#ifndef SIMULATE_PANEL_H
#define SIMULATE_PANEL_H

/******************<Summary>*****************
 * Name: simulate_panel.h
 * Purpose:
 *      > Simulation page: measured value vs the value the system is really
 *          using, and the controls (overrides) that feed simulated values for
 *          every cell, the battery voltage, and the temperature
 *      > Overrides need DEBUG or SILENT mode on the RP2350
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QLabel>

#include "status_chips.h"
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QVector>
#include <QWidget>

#include "protocol.h"
#include "serial_link.h"

class SimulatePanel : public QWidget {
    Q_OBJECT

public:
    explicit SimulatePanel(SerialLink* link, QWidget* parent = nullptr);

public slots:
    void onFrame(const proto::BatteryFrame& frame);
    void setConnectionState(bool connected, const QString& mode);

signals:
    void requestDebugMode();
    void requestPoll();

private slots:
    void onClearAll();

private:
    struct Row {
        QString label;
        QString command;            // "cell 1", "batt", "temp"
        int bit = 0;                // bit in ovr_mask
        double lo = 0.0;
        double hi = 0.0;
        int decimals = 3;
        QString unit;
        bool populated = true;
        QLabel* measured = nullptr;
        QLabel* used = nullptr;
        QCheckBox* sim = nullptr;
        QSlider* slider = nullptr;
        QDoubleSpinBox* spin = nullptr;
        QTimer* sendTimer = nullptr;
        QElapsedTimer touched;
        bool touchedValid = false;
        double lastUsed = 0.0;
    };

    SerialLink* m_link;
    QVector<Row> m_rows;

    StatusChips* m_chips;
    QLabel* m_info;
    QLabel* m_hint;
    QLabel* m_status;
    QPushButton* m_debugBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_pollBtn;

    bool m_connected = false;
    bool m_available = false;
    QString m_mode = QStringLiteral("normal");

    void buildRow(int index, const QString& label, const QString& command, int bit, double lo, double hi,
                  int decimals, const QString& unit, bool populated);
    void sendOverride(int index);
    void sendClear(int index);
    void applySimulationAvailability();
    void refreshRowEnables();
    void setStatus(const QString& text, bool error);
    QString formatValue(const Row& row, double value) const;
};

#endif // SIMULATE_PANEL_H
