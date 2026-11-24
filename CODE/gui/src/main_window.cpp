#include "main_window.h"

#include "branding.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QEvent>
#include <QMouseEvent>
#include <QUrl>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>

namespace {
const int kLogoSize = 44;                                       // team graphics in the toolbar (device-independent px)
const QStringList kModeNames = {QStringLiteral("normal"), QStringLiteral("debug"), QStringLiteral("silent")};
}

/*******MainWindow::MainWindow*******
 * Description
        > builds the toolbar, tabs, and connects all the pieces together
 * Arguments
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("SubjuGator 9 - Battery ORing & Monitoring V2"));
    resize(1000, 720);

    // panels
        m_monitor = new MonitorPanel(this);
        m_simulate = new SimulatePanel(&m_link, this);
        m_plots = new PlotPanel(this);
        m_settings = new SettingsPanel(&m_link, this);
        m_terminal = new TerminalPanel(&m_link, this);
        m_serialSettings = new SerialSettingsPanel(this);

        m_tabs = new QTabWidget(this);
        m_tabs->addTab(m_monitor, tr("Monitor"));
        m_tabs->addTab(m_plots, tr("Plots"));
        m_tabs->addTab(m_simulate, tr("Simulate"));
        m_tabs->addTab(m_settings, tr("Settings (EEPROM)"));
        m_tabs->addTab(m_terminal, tr("Terminal"));
        m_tabs->addTab(m_serialSettings, tr("Serial"));
        setCentralWidget(m_tabs);

    // toolbar
        QToolBar* bar = addToolBar(tr("Connection"));
        bar->setMovable(false);

        bar->addWidget(new QLabel(tr(" Port: "), this));
        m_portCombo = new QComboBox(this);
        m_portCombo->setMinimumWidth(260);
        m_portCombo->setEditable(true);
        m_portCombo->setToolTip(tr("Pick a port, or type a port name / device path yourself.\n"
                                   "To connect over the network (for example through an SSH tunnel to a Jetson) type tcp://host:port, such as tcp://localhost:5000."));
        bar->addWidget(m_portCombo);

        // fixed gap so the (editable) port box never touches the Refresh button
        auto* portGap = new QWidget(this);
        portGap->setFixedWidth(8);
        bar->addWidget(portGap);

        auto* refreshBtn = new QPushButton(tr("Refresh"), this);
        bar->addWidget(refreshBtn);

        m_connectBtn = new QPushButton(tr("Connect"), this);
        m_connectBtn->setMinimumWidth(100);
        m_connectBtn->setStyleSheet(QStringLiteral("QPushButton {font-weight: bold;}"));
        bar->addWidget(m_connectBtn);

        m_readOnlyBox = new QCheckBox(tr("Read-only"), this);
        m_readOnlyBox->setToolTip(tr("Listen only: the GUI sends nothing to the RP2350 (no settings, simulation, mode changes or polling).\n"
                                     "Use it while another program, such as the Jetson software, is talking to the board.\n"
                                     "The Monitor and Plots tabs work from the status lines the RP2350 sends by itself in Normal mode."));
        m_readOnlyBox->setChecked(QSettings().value(QStringLiteral("readOnly"), false).toBool());
        bar->addWidget(m_readOnlyBox);

        bar->addSeparator();
        bar->addWidget(new QLabel(tr(" Mode: "), this));

        m_modeGroup = new QButtonGroup(this);
        m_modeGroup->setExclusive(true);
        const QStringList labels = {tr("Normal"), tr("Debug"), tr("Silent")};
        const QStringList tips = {
            tr("RP2350 sends a slow status line (for the Jetson). Simulation is not allowed."),
            tr("RP2350 streams fast telemetry. Simulation allowed."),
            tr("RP2350 sends nothing by itself, so the terminal stays still. Simulation allowed.")};
        for (int i = 0; i < 3; i++){
            m_modeButtons[i] = new QPushButton(labels[i], this);
            m_modeButtons[i]->setCheckable(true);
            m_modeButtons[i]->setToolTip(tips[i]);
            m_modeGroup->addButton(m_modeButtons[i], i);
            bar->addWidget(m_modeButtons[i]);
        }

        bar->addSeparator();
        m_buzzerBtn = new QPushButton(this);
        m_buzzerBtn->setCheckable(true);
        m_buzzerBtn->setMinimumWidth(110);
        bar->addWidget(m_buzzerBtn);
        updateBuzzerButton(true);

        bar->addSeparator();
        m_pollBtn = new QPushButton(tr("Read now"), this);
        m_pollBtn->setToolTip(tr("Ask the RP2350 for one data frame."));
        bar->addWidget(m_pollBtn);

        // team graphics, pushed to the top-right corner
        auto* spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        bar->addWidget(spacer);

        m_spriteLabel = new QLabel(this);
        m_logoLabel = new QLabel(this);
        for (QLabel* label : {m_spriteLabel, m_logoLabel}){
            label->setFixedSize(kLogoSize + 6, kLogoSize);
            label->setAlignment(Qt::AlignCenter);
            label->setCursor(Qt::PointingHandCursor);
            label->installEventFilter(this);            // clicking opens the website
            bar->addWidget(label);
        }
        m_spriteLabel->setToolTip(tr("Open radm.cc in your web browser"));
        m_logoLabel->setToolTip(tr("Open mil.ufl.edu in your web browser"));
        updateLogos();

    // status bar
        m_statusConn = new QLabel(tr("Not connected"), this);
        m_statusInfo = new QLabel(this);
        statusBar()->addWidget(m_statusConn, 1);
        statusBar()->addPermanentWidget(m_statusInfo);

        auto* credit = new QLabel(tr("Made by Russell and Claude"), this);
        credit->setEnabled(false);                      // dimmed text
        statusBar()->addPermanentWidget(credit);

    // wiring
        connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshPorts);
        connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
        connect(m_modeGroup, &QButtonGroup::idClicked, this, &MainWindow::onModeButton);
        connect(m_buzzerBtn, &QPushButton::clicked, this, &MainWindow::onBuzzerToggled);
        connect(m_pollBtn, &QPushButton::clicked, this, &MainWindow::pollNow);
        connect(m_monitor, &MonitorPanel::unlatchRequested, this, [this](){
            m_link.sendCommand(QStringLiteral("unlatch"), SerialLink::Source::Gui, [this](bool ok, const QString& text){
                if (!ok){QMessageBox::warning(this, tr("RP2350"), tr("Unlatch failed: %1").arg(text));}
                pollNow();
            });
        });

        connect(&m_link, &SerialLink::connectionChanged, this, &MainWindow::onConnectionChanged);
        connect(&m_link, &SerialLink::lineReceived, this, &MainWindow::onLineReceived);
        connect(&m_link, &SerialLink::errorMessage, m_terminal, &TerminalPanel::appendNote);

        connect(m_simulate, &SimulatePanel::requestDebugMode, this, [this](){requestMode(QStringLiteral("debug"));});
        connect(m_simulate, &SimulatePanel::requestPoll, this, &MainWindow::pollNow);
        connect(m_settings, &SettingsPanel::activeValuesChanged, m_monitor, &MonitorPanel::setActiveSettings);
        connect(m_terminal, &TerminalPanel::typedCommandFinished, this, &MainWindow::onTypedCommandFinished);

        connect(m_serialSettings, &SerialSettingsPanel::configChanged, this, &MainWindow::applyLinkOptions);
        connect(m_serialSettings, &SerialSettingsPanel::refreshRequested, this, &MainWindow::refreshInfo);
        connect(m_serialSettings, &SerialSettingsPanel::applyStreamRequested, this, [this](int ms){
            m_link.sendCommand(QStringLiteral("stream %1").arg(ms), SerialLink::Source::Gui, [this](bool ok, const QString& text){
                if (ok){refreshInfo();}
                else {QMessageBox::warning(this, tr("Stream period"), text);}
            });
        });
        connect(m_serialSettings, &SerialSettingsPanel::applyRateRequested, this, [this](int ms){
            m_link.sendCommand(QStringLiteral("rate %1").arg(ms), SerialLink::Source::Gui, [this](bool ok, const QString& text){
                if (ok){refreshInfo();}
                else {QMessageBox::warning(this, tr("Measurement interval"), text);}
            });
        });

        connect(&m_keepAlive, &QTimer::timeout, this, &MainWindow::onKeepAlive);
        m_keepAlive.start(1000);

    refreshPorts();
    applyLinkOptions();
    setMode(QStringLiteral("normal"));
    setModeEnabled(false);
    m_settings->setConnected(false);
}



/*******MainWindow::updateLogos*******
 * Description
        > shows the light-mode or dark-mode team graphics, depending on the current theme
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::updateLogos(){
    const bool dark = branding::isDarkTheme(palette());
    const qreal dpr = devicePixelRatioF();

    m_spriteLabel->setPixmap(branding::renderThemedLogo(QStringLiteral("radm_sprite"), dark, kLogoSize, dpr));
    m_logoLabel->setPixmap(branding::renderThemedLogo(QStringLiteral("mil_logo"), dark, kLogoSize, dpr));
}



/*******MainWindow::eventFilter*******
 * Description
        > opens the team websites when a top-right graphic is clicked
            (radm sprite -> https://radm.cc, MIL logo -> https://mil.ufl.edu)
 * Arguments
        > QObject* watched: object that got the event
        > QEvent* event: the event
 * Returns
        > bool: true=event handled
*/
bool MainWindow::eventFilter(QObject* watched, QEvent* event){
    if (event->type() == QEvent::MouseButtonRelease && (watched == m_spriteLabel || watched == m_logoLabel)){
        auto* me = static_cast<QMouseEvent*>(event);
        QLabel* label = static_cast<QLabel*>(watched);

        if (me->button() == Qt::LeftButton && label->rect().contains(me->position().toPoint())){
            const QUrl url(watched == m_spriteLabel ? QStringLiteral("https://radm.cc") : QStringLiteral("https://mil.ufl.edu"));
            QDesktopServices::openUrl(url);
        }
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}



/*******MainWindow::changeEvent*******
 * Description
        > switches the team graphics when the theme (light / dark) changes while the program runs
 * Arguments
        > QEvent* event: the event
 * Returns
        > N/A
*/
void MainWindow::changeEvent(QEvent* event){
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ThemeChange){updateLogos();}
}



/*******MainWindow::refreshPorts*******
 * Description
        > refills the port list (Raspberry Pi USB serial ports are listed first and preselected)
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::refreshPorts(){
    QString previous = selectedPort();
    if (previous.isEmpty()){previous = QSettings().value(QStringLiteral("lastPort")).toString();}

    m_portCombo->clear();

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    std::sort(ports.begin(), ports.end(), [](const QSerialPortInfo& a, const QSerialPortInfo& b){
        bool pa = (a.vendorIdentifier() == 0x2E8A);
        bool pb = (b.vendorIdentifier() == 0x2E8A);
        if (pa != pb){return pa;}
        return a.portName() < b.portName();
    });

    for (const QSerialPortInfo& info : ports){
        QString text = info.portName();
        if (!info.description().isEmpty()){text += QStringLiteral(" - ") + info.description();}
        if (info.vendorIdentifier() == 0x2E8A){text += tr("  (Raspberry Pi)");}
        m_portCombo->addItem(text, info.portName());
    }

    if (m_portCombo->count() == 0){m_portCombo->setEditText(QString());}

    int idx = m_portCombo->findData(previous);
    if (idx >= 0){m_portCombo->setCurrentIndex(idx);}
    else if (!previous.isEmpty()){m_portCombo->setEditText(previous);}
}



/*******MainWindow::selectedPort*******
 * Description
        > gets the port name chosen in the toolbar
 * Arguments
        > N/A
 * Returns
        > QString: port name, empty if none
*/
QString MainWindow::selectedPort() const {
    int idx = m_portCombo->findText(m_portCombo->currentText());
    if (idx >= 0){return m_portCombo->itemData(idx).toString();}
    return m_portCombo->currentText().trimmed();
}



/*******MainWindow::applyLinkOptions*******
 * Description
        > pushes the Serial tab choices (framing, reconnect, timeout, keep-alive) into the link
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::applyLinkOptions(){
    m_link.applyLiveConfig(m_serialSettings->config());
    m_link.setAutoReconnect(m_serialSettings->autoReconnect());
    m_link.setCommandTimeoutMs(m_serialSettings->commandTimeoutMs());
}



/*******MainWindow::connectTo*******
 * Description
        > opens a port by name (used by the connect button and by automated tests)
 * Arguments
        > const QString& portName: port to open, for example "COM5" or "/dev/ttyACM0"
 * Returns
        > bool: true=opened
*/
bool MainWindow::connectTo(const QString& target, bool readOnly){
    SerialLink::Config cfg = m_serialSettings->config();
    cfg.portName = target;
    cfg.readOnly = readOnly;

    // "tcp://host:port" = network connection
        if (target.startsWith(QLatin1String("tcp://"), Qt::CaseInsensitive)){
            const QString hostPort = target.mid(6);
            int colon = hostPort.lastIndexOf(QLatin1Char(':'));
            bool portOk = false;
            int port = (colon > 0) ? hostPort.mid(colon + 1).toInt(&portOk) : 0;

            if (!portOk || port < 1 || port > 65535){
                QMessageBox::warning(this, tr("Connect"), tr("Use the form tcp://host:port, for example tcp://localhost:5000."));
                return false;
            }

            cfg.tcp = true;
            cfg.host = hostPort.left(colon);
            cfg.tcpPort = (quint16)port;
        }

    QString error;
    if (!m_link.open(cfg, &error)){
        m_terminal->appendNote(tr("could not open %1: %2").arg(target, error));
        QMessageBox::warning(this, tr("Connect"), tr("Could not open %1:\n%2").arg(target, error));
        return false;
    }

    QSettings().setValue(QStringLiteral("lastPort"), target);
    QSettings().setValue(QStringLiteral("readOnly"), readOnly);
    return true;
}



/*******MainWindow::onConnectClicked*******
 * Description
        > Connect / Disconnect button
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::onConnectClicked(){
    if (m_link.isOpen()){
        m_link.close();
        return;
    }

    QString port = selectedPort();
    if (port.isEmpty()){
        QMessageBox::information(this, tr("Connect"), tr("No serial port selected. Plug in the RP2350 and press Refresh."));
        return;
    }

    connectTo(port, m_readOnlyBox->isChecked());
}



/*******MainWindow::onConnectionChanged*******
 * Description
        > updates every part of the UI when the port opens or closes
 * Arguments
        > bool open: true=port is open
 * Returns
        > N/A
*/
void MainWindow::onConnectionChanged(bool open){
    m_connectBtn->setText(open ? tr("Disconnect") : tr("Connect"));
    m_portCombo->setEnabled(!open);
    m_readOnlyBox->setEnabled(!open);
    setModeEnabled(open);
    m_pollBtn->setEnabled(open);

    const bool readOnly = open && m_link.isReadOnly();

    if (open){
        m_statusConn->setText(tr("Connected to %1%2").arg(m_link.portName(), readOnly ? tr("  (READ-ONLY)") : QString()));
        m_terminal->appendNote(tr("connected to %1%2").arg(m_link.portName(), readOnly ? tr(" (read-only, nothing will be sent)") : QString()));
        if (readOnly){m_statusInfo->setText(tr("listening only - data appears when the RP2350 sends status lines (Normal mode)"));}
        else {handshake();}
    }
    else {
        m_statusConn->setText(tr("Not connected"));
        m_statusInfo->clear();
        m_terminal->appendNote(tr("disconnected"));
        m_fw.clear();
        setMode(QStringLiteral("normal"));
    }

    m_settings->setConnected(open);
    m_monitor->setConnectionState(open);
    m_plots->setConnectionState(open);
    m_simulate->setConnectionState(open, m_mode);
    m_serialSettings->setConnected(open, m_mode);

    applyReadOnlyUi(readOnly);
}



/*******MainWindow::applyReadOnlyUi*******
 * Description
        > disables everything that would send a command to the RP2350 while the connection is read-only,
            and re-enables it otherwise
 * Arguments
        > bool readOnly: true=read-only connection
 * Returns
        > N/A
*/
void MainWindow::applyReadOnlyUi(bool readOnly){
    m_monitor->setReadOnly(readOnly);
    m_simulate->setEnabled(!readOnly);
    m_settings->setEnabled(!readOnly);

    if (readOnly){
        setModeEnabled(false);
        m_pollBtn->setEnabled(false);
        m_buzzerBtn->setEnabled(false);
        m_serialSettings->setConnected(false, m_mode);
    }
}



/*******MainWindow::handshake*******
 * Description
        > right after connecting: asks the RP2350 for its version, mode, and settings
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::handshake(){
    m_link.sendCommand(QStringLiteral("ver"), SerialLink::Source::Gui, [this](bool ok, const QString& text){
        if (!ok){
            m_statusInfo->setText(tr("no answer to \"ver\" - is this the battery RP2350?"));
            return;
        }

        const QMap<QString, QString> kv = proto::parseKeyValues(text);
        m_fw = tr("v%1 (protocol %2)").arg(kv.value(QStringLiteral("fw")), kv.value(QStringLiteral("proto")));
        m_statusInfo->setText(tr("firmware %1").arg(m_fw));
        refreshInfo();
    });

    m_settings->refresh();
}



/*******MainWindow::setModeEnabled*******
 * Description
        > enables or disables the mode buttons
 * Arguments
        > bool enabled: true=usable
 * Returns
        > N/A
*/
void MainWindow::setModeEnabled(bool enabled){
    for (QPushButton* b : m_modeButtons){b->setEnabled(enabled);}
}



/*******MainWindow::setMode*******
 * Description
        > records the RP2350's mode and updates the toolbar and panels
 * Arguments
        > const QString& mode: "normal", "debug", or "silent"
 * Returns
        > N/A
*/
void MainWindow::setMode(const QString& mode){
    int idx = kModeNames.indexOf(mode);
    if (idx < 0){return;}

    m_mode = mode;

    m_modeGroup->setExclusive(false);
    for (int i = 0; i < 3; i++){m_modeButtons[i]->setChecked(i == idx);}
    m_modeGroup->setExclusive(true);

    m_simulate->setConnectionState(m_link.isOpen(), m_mode);
    m_serialSettings->setConnected(m_link.isOpen(), m_mode);

    // the buzzers are always on in Normal mode, they can only be muted in Debug / Silent mode
    if (m_mode == QLatin1String("normal")){updateBuzzerButton(true);}
    else {updateBuzzerButton(m_buzzerBtn->isChecked());}
}



/*******MainWindow::currentMode*******
 * Description
        > gets the RP2350 mode the GUI believes is active
 * Arguments
        > N/A
 * Returns
        > QString: "normal", "debug", or "silent"
*/
QString MainWindow::currentMode() const {
    return m_mode;
}



/*******MainWindow::onModeButton*******
 * Description
        > a mode button was clicked: ask the RP2350 to switch
 * Arguments
        > int id: 0=normal 1=debug 2=silent
 * Returns
        > N/A
*/
void MainWindow::onModeButton(int id){
    requestMode(kModeNames.value(id));
}



/*******MainWindow::updateBuzzerButton*******
 * Description
        > shows whether the RP2350's buzzers are on or muted and enables the button
            only in Debug / Silent mode (the buzzers are always on in Normal mode)
 * Arguments
        > bool enabled: true=buzzers on, false=buzzers muted
 * Returns
        > N/A
*/
void MainWindow::updateBuzzerButton(bool enabled){
    m_buzzerBtn->setChecked(enabled);
    m_buzzerBtn->setText(enabled ? tr("Buzzers: ON") : tr("Buzzers: MUTED"));
    m_buzzerBtn->setStyleSheet(enabled ? QString() : QStringLiteral("QPushButton {background-color: #ffcc80; font-weight: bold;}"));
    m_buzzerBtn->setEnabled(m_link.isOpen() && m_mode != QLatin1String("normal"));
    m_buzzerBtn->setToolTip(m_mode == QLatin1String("normal")
        ? tr("The buzzers are always on in Normal mode. Switch to Debug or Silent mode to mute them.")
        : tr("Click to mute / un-mute the RP2350's buzzers (Debug and Silent mode only)."));
}



/*******MainWindow::onBuzzerToggled*******
 * Description
        > the buzzer button was clicked: asks the RP2350 to un-mute / mute its buzzers
 * Arguments
        > bool enabled: true=buzzers should sound, false=mute them
 * Returns
        > N/A
*/
void MainWindow::onBuzzerToggled(bool enabled){
    m_link.sendCommand(enabled ? QStringLiteral("buzzer on") : QStringLiteral("buzzer off"), SerialLink::Source::Gui,
        [this](bool ok, const QString& text){
            if (!ok){QMessageBox::warning(this, tr("Buzzers"), text);}
            refreshInfo();      // always re-sync the button with the real state
        });
}



/*******MainWindow::requestMode*******
 * Description
        > sends "mode <name>" and updates the UI from the RP2350's answer
 * Arguments
        > const QString& mode: "normal", "debug", or "silent"
 * Returns
        > N/A
*/
void MainWindow::requestMode(const QString& mode){
    QString cmd = QStringLiteral("mode ") + mode;
    if (mode == QLatin1String("debug") && m_serialSettings->streamMs() > 0){cmd += QStringLiteral(" %1").arg(m_serialSettings->streamMs());}
    else if (mode == QLatin1String("debug")){cmd += QStringLiteral(" 0");}

    m_link.sendCommand(cmd, SerialLink::Source::Gui, [this](bool ok, const QString& text){
        if (!ok){
            QMessageBox::warning(this, tr("Mode"), text);
        }

        refreshInfo();      // always re-sync the buttons with the real mode
    });
}



/*******MainWindow::parseInfoReply*******
 * Description
        > reads the RP2350's "info" reply and updates the mode, Serial tab and status bar
 * Arguments
        > const QString& text: reply text after "OK"
 * Returns
        > N/A
*/
void MainWindow::parseInfoReply(const QString& text){
    const QMap<QString, QString> kv = proto::parseKeyValues(text);

    QString mode = kv.value(QStringLiteral("mode"));
    int stream = kv.contains(QStringLiteral("stream_ms")) ? kv.value(QStringLiteral("stream_ms")).toInt() : -1;
    int check = kv.contains(QStringLiteral("check_ms")) ? kv.value(QStringLiteral("check_ms")).toInt() : -1;

    if (!mode.isEmpty()){setMode(mode);}
    if (kv.contains(QStringLiteral("buzzers"))){updateBuzzerButton(kv.value(QStringLiteral("buzzers")) != QLatin1String("0"));}
    m_serialSettings->setDeviceInfo(m_mode, stream, check, m_fw);

    QString info = m_fw.isEmpty() ? QString() : tr("firmware %1   ").arg(m_fw);
    info += tr("mode %1   stream %2 ms   measure every %3 ms").arg(m_mode).arg(stream).arg(check);
    m_statusInfo->setText(info);
}



/*******MainWindow::refreshInfo*******
 * Description
        > asks the RP2350 for its mode and timing ("info")
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::refreshInfo(){
    m_link.sendCommand(QStringLiteral("info"), SerialLink::Source::Gui, [this](bool ok, const QString& text){
        if (ok){
            parseInfoReply(text);
            if (m_mode == QLatin1String("silent")){pollNow();}
        }
    });
}



/*******MainWindow::pollNow*******
 * Description
        > asks for one data frame ("status" in Normal mode, "telemetry" otherwise)
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::pollNow(){
    QString cmd = (m_mode == QLatin1String("normal")) ? QStringLiteral("status") : QStringLiteral("telemetry");
    m_link.sendCommand(cmd, SerialLink::Source::Gui, {}, true);
}



/*******MainWindow::onKeepAlive*******
 * Description
        > once a second: sends "ping" when due so the RP2350 does not clear simulated values
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void MainWindow::onKeepAlive(){
    static qint64 lastPing = 0;

    int interval = m_serialSettings->keepAliveMs();
    if (interval <= 0 || !m_link.isOpen() || m_mode == QLatin1String("normal")){return;}

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastPing < interval){return;}

    lastPing = now;
    m_link.sendCommand(QStringLiteral("ping"), SerialLink::Source::Background);
}



/*******MainWindow::onTypedCommandFinished*******
 * Description
        > after a command typed in the terminal, refreshes whatever it may have changed
 * Arguments
        > const QString& firstWord: first word of the typed command
 * Returns
        > N/A
*/
void MainWindow::onTypedCommandFinished(const QString& firstWord){
    static const QStringList settingsWords = {"set", "save", "reload", "discard", "defaults"};
    static const QStringList stateWords = {"mode", "ovr", "rate", "stream", "buzzer"};

    if (settingsWords.contains(firstWord)){m_settings->refresh();}
    if (stateWords.contains(firstWord)){refreshInfo();}
}



/*******MainWindow::onLineReceived*******
 * Description
        > handles lines that carry data for the GUI: S/T frames and EVT messages
 * Arguments
        > const QString& line: received line
        > bool background: unused
 * Returns
        > N/A
*/
void MainWindow::onLineReceived(const QString& line, bool background){
    Q_UNUSED(background);

    if (line.startsWith(QLatin1String("S,")) || line.startsWith(QLatin1String("T,"))){
        proto::BatteryFrame frame;
        if (proto::parseFrame(line, &frame)){
            m_monitor->onFrame(frame);
            m_simulate->onFrame(frame);
            m_plots->onFrame(frame);
        }
        return;
    }

    if (line.startsWith(QLatin1String("EVT "))){
        // the RP2350 changed something by itself (disconnect, override timeout)
        refreshInfo();
    }
}
