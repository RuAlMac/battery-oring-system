#ifndef SERIAL_SETTINGS_PANEL_H
#define SERIAL_SETTINGS_PANEL_H

/******************<Summary>*****************
 * Name: serial_settings_panel.h
 * Purpose:
 *      > Settings of the serial connection itself (baud, framing, DTR/RTS,
 *          line ending, reconnect, keep-alive) - saved between runs
 *      > Settings of the RP2350's serial interface (streaming period,
 *          battery measurement interval) - these live in the RP2350's RAM and
 *          are NOT stored in the EEPROM
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

#include "serial_link.h"

class SerialSettingsPanel : public QWidget {
    Q_OBJECT

public:
    explicit SerialSettingsPanel(QWidget* parent = nullptr);

    SerialLink::Config config() const;
    bool autoReconnect() const;
    int keepAliveMs() const;
    int commandTimeoutMs() const;
    int streamMs() const;
    void setDeviceInfo(const QString& mode, int streamMs, int checkMs, const QString& fw);
    void setConnected(bool connected, const QString& mode);

signals:
    void configChanged();
    void applyStreamRequested(int ms);
    void applyRateRequested(int ms);
    void refreshRequested();

private:
    QComboBox* m_baud;
    QComboBox* m_dataBits;
    QComboBox* m_parity;
    QComboBox* m_stopBits;
    QComboBox* m_flow;
    QCheckBox* m_dtr;
    QCheckBox* m_rts;
    QComboBox* m_eol;
    QCheckBox* m_autoReconnect;
    QSpinBox* m_keepAlive;
    QSpinBox* m_timeout;

    QLabel* m_deviceMode;
    QLabel* m_deviceFw;
    QSpinBox* m_stream;
    QSpinBox* m_rate;
    QPushButton* m_applyStream;
    QPushButton* m_applyRate;
    QPushButton* m_refresh;

    void loadSaved();
    void saveCurrent();
};

#endif // SERIAL_SETTINGS_PANEL_H
