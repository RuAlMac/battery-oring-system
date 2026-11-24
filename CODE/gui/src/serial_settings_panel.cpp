#include "serial_settings_panel.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

#include "protocol.h"

/*******SerialSettingsPanel::SerialSettingsPanel*******
 * Description
        > builds the connection settings and the RP2350 interface settings
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
SerialSettingsPanel::SerialSettingsPanel(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    // connection group
        auto* conn = new QGroupBox(tr("Serial connection (this computer)"), this);
        auto* form = new QFormLayout(conn);

        auto* portNote = new QLabel(tr("The port itself is chosen in the toolbar at the top. Changes below apply immediately and are remembered."), conn);
        portNote->setWordWrap(true);
        form->addRow(portNote);

        m_baud = new QComboBox(conn);
        m_baud->setEditable(true);
        for (int b : {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600}){m_baud->addItem(QString::number(b), b);}
        m_baud->setCurrentText(QStringLiteral("115200"));
        m_baud->setToolTip(tr("The board uses USB serial, which ignores the baud rate. Only matters if you use a UART adapter."));
        form->addRow(tr("Baud rate"), m_baud);

        m_dataBits = new QComboBox(conn);
        for (int d : {5, 6, 7, 8}){m_dataBits->addItem(QString::number(d), d);}
        m_dataBits->setCurrentText(QStringLiteral("8"));
        form->addRow(tr("Data bits"), m_dataBits);

        m_parity = new QComboBox(conn);
        m_parity->addItem(tr("None"), (int)QSerialPort::NoParity);
        m_parity->addItem(tr("Even"), (int)QSerialPort::EvenParity);
        m_parity->addItem(tr("Odd"), (int)QSerialPort::OddParity);
        form->addRow(tr("Parity"), m_parity);

        m_stopBits = new QComboBox(conn);
        m_stopBits->addItem(QStringLiteral("1"), (int)QSerialPort::OneStop);
        m_stopBits->addItem(QStringLiteral("2"), (int)QSerialPort::TwoStop);
        form->addRow(tr("Stop bits"), m_stopBits);

        m_flow = new QComboBox(conn);
        m_flow->addItem(tr("None"), (int)QSerialPort::NoFlowControl);
        m_flow->addItem(tr("Hardware (RTS/CTS)"), (int)QSerialPort::HardwareControl);
        m_flow->addItem(tr("Software (XON/XOFF)"), (int)QSerialPort::SoftwareControl);
        form->addRow(tr("Flow control"), m_flow);

        m_dtr = new QCheckBox(tr("Assert DTR (required: the RP2350 only sends data while DTR is on)"), conn);
        m_dtr->setChecked(true);
        form->addRow(m_dtr);

        m_rts = new QCheckBox(tr("Assert RTS"), conn);
        m_rts->setChecked(true);
        form->addRow(m_rts);

        m_eol = new QComboBox(conn);
        m_eol->addItem(tr("LF  (\\n)"), QStringLiteral("\n"));
        m_eol->addItem(tr("CR  (\\r)"), QStringLiteral("\r"));
        m_eol->addItem(tr("CR+LF  (\\r\\n)"), QStringLiteral("\r\n"));
        form->addRow(tr("Line ending sent"), m_eol);

        m_autoReconnect = new QCheckBox(tr("Reconnect automatically when the RP2350 is plugged back in"), conn);
        m_autoReconnect->setChecked(true);
        form->addRow(m_autoReconnect);

        m_keepAlive = new QSpinBox(conn);
        m_keepAlive->setRange(0, 25000);
        m_keepAlive->setSingleStep(500);
        m_keepAlive->setSuffix(tr(" ms"));
        m_keepAlive->setSpecialValueText(tr("off"));
        m_keepAlive->setValue(5000);
        m_keepAlive->setToolTip(tr("In Debug / Silent mode the RP2350 clears simulated values after 30 s without any command, so the GUI sends 'ping' this often."));
        form->addRow(tr("Keep-alive ping interval"), m_keepAlive);

        m_timeout = new QSpinBox(conn);
        m_timeout->setRange(500, 30000);
        m_timeout->setSingleStep(500);
        m_timeout->setSuffix(tr(" ms"));
        m_timeout->setValue(3000);
        form->addRow(tr("Wait for a reply up to"), m_timeout);

        root->addWidget(conn);

    // board interface group
        auto* dev = new QGroupBox(tr("RP2350 serial interface (stored in the RP2350's RAM, not the EEPROM)"), this);
        auto* devForm = new QFormLayout(dev);

        m_deviceMode = new QLabel(tr("-"), dev);
        devForm->addRow(tr("Current mode"), m_deviceMode);

        m_deviceFw = new QLabel(tr("-"), dev);
        devForm->addRow(tr("Firmware"), m_deviceFw);

        m_stream = new QSpinBox(dev);
        m_stream->setRange(0, proto::kStreamMaxMs);
        m_stream->setSuffix(tr(" ms"));
        m_stream->setSpecialValueText(tr("0 (off)"));
        m_stream->setValue(proto::kDefaultDebugStreamMs);
        m_stream->setToolTip(tr("Time between automatic data frames (0 = off, otherwise 20 to 10000 ms). Not available in Silent mode."));
        m_applyStream = new QPushButton(tr("Apply"), dev);
        auto* streamRow = new QHBoxLayout();
        streamRow->addWidget(m_stream, 1);
        streamRow->addWidget(m_applyStream);
        devForm->addRow(tr("Data frame period"), streamRow);

        m_rate = new QSpinBox(dev);
        m_rate->setRange(proto::kRateMinMs, 60000);
        m_rate->setSuffix(tr(" ms"));
        m_rate->setValue(250);
        m_rate->setToolTip(tr("How often the RP2350 measures the battery (minimum 50 ms)."));
        m_applyRate = new QPushButton(tr("Apply"), dev);
        auto* rateRow = new QHBoxLayout();
        rateRow->addWidget(m_rate, 1);
        rateRow->addWidget(m_applyRate);
        devForm->addRow(tr("Battery measurement interval"), rateRow);

        m_refresh = new QPushButton(tr("Read from RP2350"), dev);
        devForm->addRow(m_refresh);

        auto* note = new QLabel(tr("The mode (Normal / Debug / Silent) is chosen with the buttons in the toolbar. "
                                   "RP2350 settings that are kept in the EEPROM (thresholds, intervals) are on the Settings tab."), dev);
        note->setWordWrap(true);
        devForm->addRow(note);

        root->addWidget(dev);
        root->addStretch(1);

    loadSaved();

    auto changed = [this](){
        saveCurrent();
        emit configChanged();
    };
    connect(m_baud, &QComboBox::currentTextChanged, this, changed);
    connect(m_dataBits, &QComboBox::currentIndexChanged, this, changed);
    connect(m_parity, &QComboBox::currentIndexChanged, this, changed);
    connect(m_stopBits, &QComboBox::currentIndexChanged, this, changed);
    connect(m_flow, &QComboBox::currentIndexChanged, this, changed);
    connect(m_eol, &QComboBox::currentIndexChanged, this, changed);
    connect(m_dtr, &QCheckBox::toggled, this, changed);
    connect(m_rts, &QCheckBox::toggled, this, changed);
    connect(m_autoReconnect, &QCheckBox::toggled, this, changed);
    connect(m_keepAlive, QOverload<int>::of(&QSpinBox::valueChanged), this, changed);
    connect(m_timeout, QOverload<int>::of(&QSpinBox::valueChanged), this, changed);

    connect(m_applyStream, &QPushButton::clicked, this, [this](){emit applyStreamRequested(m_stream->value());});
    connect(m_applyRate, &QPushButton::clicked, this, [this](){emit applyRateRequested(m_rate->value());});
    connect(m_refresh, &QPushButton::clicked, this, &SerialSettingsPanel::refreshRequested);

    setConnected(false, QStringLiteral("normal"));
}



/*******SerialSettingsPanel::loadSaved*******
 * Description
        > restores the connection settings used last time
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SerialSettingsPanel::loadSaved(){
    QSettings s;
    s.beginGroup(QStringLiteral("serial"));

    m_baud->setCurrentText(s.value(QStringLiteral("baud"), 115200).toString());
    m_dataBits->setCurrentText(s.value(QStringLiteral("dataBits"), 8).toString());
    m_parity->setCurrentIndex(qBound(0, s.value(QStringLiteral("parity"), 0).toInt(), m_parity->count() - 1));
    m_stopBits->setCurrentIndex(qBound(0, s.value(QStringLiteral("stopBits"), 0).toInt(), m_stopBits->count() - 1));
    m_flow->setCurrentIndex(qBound(0, s.value(QStringLiteral("flow"), 0).toInt(), m_flow->count() - 1));
    m_eol->setCurrentIndex(qBound(0, s.value(QStringLiteral("eol"), 0).toInt(), m_eol->count() - 1));
    m_dtr->setChecked(s.value(QStringLiteral("dtr"), true).toBool());
    m_rts->setChecked(s.value(QStringLiteral("rts"), true).toBool());
    m_autoReconnect->setChecked(s.value(QStringLiteral("autoReconnect"), true).toBool());
    m_keepAlive->setValue(s.value(QStringLiteral("keepAliveMs"), 5000).toInt());
    m_timeout->setValue(s.value(QStringLiteral("timeoutMs"), 3000).toInt());
    m_stream->setValue(s.value(QStringLiteral("streamMs"), proto::kDefaultDebugStreamMs).toInt());
}



/*******SerialSettingsPanel::saveCurrent*******
 * Description
        > remembers the connection settings for next time
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SerialSettingsPanel::saveCurrent(){
    QSettings s;
    s.beginGroup(QStringLiteral("serial"));

    s.setValue(QStringLiteral("baud"), m_baud->currentText());
    s.setValue(QStringLiteral("dataBits"), m_dataBits->currentText());
    s.setValue(QStringLiteral("parity"), m_parity->currentIndex());
    s.setValue(QStringLiteral("stopBits"), m_stopBits->currentIndex());
    s.setValue(QStringLiteral("flow"), m_flow->currentIndex());
    s.setValue(QStringLiteral("eol"), m_eol->currentIndex());
    s.setValue(QStringLiteral("dtr"), m_dtr->isChecked());
    s.setValue(QStringLiteral("rts"), m_rts->isChecked());
    s.setValue(QStringLiteral("autoReconnect"), m_autoReconnect->isChecked());
    s.setValue(QStringLiteral("keepAliveMs"), m_keepAlive->value());
    s.setValue(QStringLiteral("timeoutMs"), m_timeout->value());
    s.setValue(QStringLiteral("streamMs"), m_stream->value());
}



/*******SerialSettingsPanel::config*******
 * Description
        > gets the connection settings chosen by the user (port name left empty)
 * Arguments
        > N/A
 * Returns
        > SerialLink::Config: baud, framing, flow control, DTR, RTS, line ending
*/
SerialLink::Config SerialSettingsPanel::config() const {
    SerialLink::Config c;

    bool ok = false;
    int baud = m_baud->currentText().toInt(&ok);
    c.baud = (ok && baud > 0) ? baud : 115200;
    c.dataBits = (QSerialPort::DataBits)m_dataBits->currentData().toInt();
    c.parity = (QSerialPort::Parity)m_parity->currentData().toInt();
    c.stopBits = (QSerialPort::StopBits)m_stopBits->currentData().toInt();
    c.flow = (QSerialPort::FlowControl)m_flow->currentData().toInt();
    c.dtr = m_dtr->isChecked();
    c.rts = m_rts->isChecked();
    c.eol = m_eol->currentData().toString();

    return c;
}



/*******SerialSettingsPanel::autoReconnect*******
 * Description
        > gets the auto-reconnect choice
 * Arguments
        > N/A
 * Returns
        > bool: true=reconnect automatically
*/
bool SerialSettingsPanel::autoReconnect() const {
    return m_autoReconnect->isChecked();
}



/*******SerialSettingsPanel::keepAliveMs*******
 * Description
        > gets the keep-alive ping interval
 * Arguments
        > N/A
 * Returns
        > int: milliseconds, 0 = off
*/
int SerialSettingsPanel::keepAliveMs() const {
    return m_keepAlive->value();
}



/*******SerialSettingsPanel::commandTimeoutMs*******
 * Description
        > gets how long to wait for a reply
 * Arguments
        > N/A
 * Returns
        > int: milliseconds
*/
int SerialSettingsPanel::commandTimeoutMs() const {
    return m_timeout->value();
}



/*******SerialSettingsPanel::streamMs*******
 * Description
        > gets the data frame period chosen for Debug mode
 * Arguments
        > N/A
 * Returns
        > int: milliseconds (0 = off)
*/
int SerialSettingsPanel::streamMs() const {
    return m_stream->value();
}



/*******SerialSettingsPanel::setDeviceInfo*******
 * Description
        > shows what the RP2350 reported (mode, periods, firmware)
 * Arguments
        > const QString& mode: "normal", "debug", or "silent"
        > int streamMs: current frame period, -1 if unknown
        > int checkMs: current measurement interval, -1 if unknown
        > const QString& fw: firmware text, empty if unknown
 * Returns
        > N/A
*/
void SerialSettingsPanel::setDeviceInfo(const QString& mode, int streamMs, int checkMs, const QString& fw){
    m_deviceMode->setText(mode);
    if (!fw.isEmpty()){m_deviceFw->setText(fw);}
    if (streamMs >= 0 && mode == QLatin1String("debug")){m_stream->setValue(streamMs);}
    if (checkMs >= proto::kRateMinMs){m_rate->setValue(checkMs);}
}



/*******SerialSettingsPanel::setConnected*******
 * Description
        > enables the RP2350-interface controls only while connected
 * Arguments
        > bool connected: true=port open
        > const QString& mode: current RP2350 mode
 * Returns
        > N/A
*/
void SerialSettingsPanel::setConnected(bool connected, const QString& mode){
    m_deviceMode->setText(connected ? mode : QStringLiteral("-"));
    if (!connected){m_deviceFw->setText(QStringLiteral("-"));}

    m_applyStream->setEnabled(connected && mode != QLatin1String("silent"));
    m_applyRate->setEnabled(connected);
    m_refresh->setEnabled(connected);
}
