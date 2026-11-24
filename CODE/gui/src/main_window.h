#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

/******************<Summary>*****************
 * Name: main_window.h
 * Purpose:
 *      > Main window: toolbar (port, connect, mode, read now), tabs for
 *          Monitor & Simulate / Settings (EEPROM) / Terminal / Serial, and the
 *          glue between them
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QCheckBox>
#include <QTabWidget>
#include <QTimer>

#include "monitor_panel.h"
#include "plot_panel.h"
#include "serial_link.h"
#include "simulate_panel.h"
#include "serial_settings_panel.h"
#include "settings_panel.h"
#include "terminal_panel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    // used by the smoke test / automation
    bool connectTo(const QString& target, bool readOnly = false);
    QString currentMode() const;

protected:
    void changeEvent(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onConnectClicked();
    void onConnectionChanged(bool open);
    void onLineReceived(const QString& line, bool background);
    void onModeButton(int id);
    void onBuzzerToggled(bool enabled);
    void onKeepAlive();
    void refreshPorts();
    void refreshInfo();
    void pollNow();
    void onTypedCommandFinished(const QString& firstWord);

private:
    SerialLink m_link;
    QComboBox* m_portCombo;
    QPushButton* m_connectBtn;
    QCheckBox* m_readOnlyBox;
    QButtonGroup* m_modeGroup;
    QPushButton* m_modeButtons[3];
    QPushButton* m_buzzerBtn;
    QPushButton* m_pollBtn;
    QTabWidget* m_tabs;

    MonitorPanel* m_monitor;
    SimulatePanel* m_simulate;
    PlotPanel* m_plots;
    SettingsPanel* m_settings;
    TerminalPanel* m_terminal;
    SerialSettingsPanel* m_serialSettings;

    QLabel* m_spriteLabel;
    QLabel* m_logoLabel;
    QLabel* m_statusConn;
    QLabel* m_statusInfo;
    QTimer m_keepAlive;

    QString m_mode = QStringLiteral("normal");
    QString m_fw;

    void setMode(const QString& mode);
    void setModeEnabled(bool enabled);
    void updateBuzzerButton(bool enabled);
    void applyLinkOptions();
    void handshake();
    void parseInfoReply(const QString& text);
    void requestMode(const QString& mode);
    QString selectedPort() const;
    void applyReadOnlyUi(bool readOnly);
    void updateLogos();
};

#endif // MAIN_WINDOW_H
