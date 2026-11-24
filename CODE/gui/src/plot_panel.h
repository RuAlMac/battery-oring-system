#ifndef PLOT_PANEL_H
#define PLOT_PANEL_H

/******************<Summary>*****************
 * Name: plot_panel.h
 * Purpose:
 *      > Plots page: records the battery values the GUI receives and plots them over time
 *          - one plot per value (4 cells, battery voltage, temperature)
 *          - one large overlay plot of the values ticked above it
 *          - Start / Stop recording, Clear, Save CSV, selectable time window
 *      > The firmware keeps no history, so everything here lives in the GUI only
 *      > Plotted values are the ones the RP2350 is using (simulated values included)
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <array>

#include "plot_widgets.h"
#include "protocol.h"

class PlotPanel : public QWidget {
    Q_OBJECT

public:
    explicit PlotPanel(QWidget* parent = nullptr);

    int sampleCount() const {return m_data.size();}
    bool isRecording() const {return m_recording;}

public slots:
    void onFrame(const proto::BatteryFrame& frame);
    void setConnectionState(bool connected);

private slots:
    void onRecordToggled(bool on);
    void clearData();
    void saveCsv();
    void updateOverlaySeries();
    void updateWindow();

private:
    PlotData m_data;
    bool m_recording = false;
    bool m_connected = false;
    bool m_gapPending = false;          // next sample starts after a pause, break the lines
    double m_timeBaseMs = 0.0;          // device uptime that corresponds to t = 0
    qint64 m_lastDeviceMs = -1;

    QPushButton* m_recordBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_saveBtn;
    QComboBox* m_window;
    QLabel* m_status;
    std::array<QCheckBox*, plotdata::kSeriesCount> m_overlayChecks;
    std::array<PlotWidget*, plotdata::kSeriesCount> m_plots;
    PlotWidget* m_overlay;

    void refreshPlots();
    void refreshStatus();
    QWidget* makeBox(const QString& title, QWidget* content);
};

#endif // PLOT_PANEL_H
