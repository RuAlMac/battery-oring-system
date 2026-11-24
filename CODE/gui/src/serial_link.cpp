#include "serial_link.h"

#include <QSerialPortInfo>

/*******SerialLink::SerialLink*******
 * Description
        > creates the link and starts the timer used for reply timeouts and reconnecting
 * Arguments
        > QObject* parent: Qt parent
 * Returns
        > N/A
*/
SerialLink::SerialLink(QObject* parent) : QObject(parent) {
    connect(&m_port, &QSerialPort::readyRead, this, &SerialLink::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, &SerialLink::onPortError);
    connect(&m_sock, &QTcpSocket::readyRead, this, &SerialLink::onReadyRead);
    connect(&m_sock, &QTcpSocket::disconnected, this, &SerialLink::onSocketDisconnected);
    connect(&m_tick, &QTimer::timeout, this, &SerialLink::onTick);
    m_tick.start(200);
}



/*******SerialLink::applyPortSettings*******
 * Description
        > applies baud, data bits, parity, stop bits, flow control, DTR and RTS to the port
            (works while the port is open)
 * Arguments
        > const Config& cfg: settings to apply
 * Returns
        > N/A
*/
void SerialLink::applyPortSettings(const Config& cfg){
    m_port.setBaudRate(cfg.baud);
    m_port.setDataBits(cfg.dataBits);
    m_port.setParity(cfg.parity);
    m_port.setStopBits(cfg.stopBits);
    m_port.setFlowControl(cfg.flow);

    if (m_port.isOpen()){
        if (cfg.flow != QSerialPort::HardwareControl){m_port.setRequestToSend(cfg.rts);}
        m_port.setDataTerminalReady(cfg.dtr);
    }
}



/*******SerialLink::open*******
 * Description
        > opens the serial port
 * Arguments
        > const Config& cfg: port name and settings
        > QString* error: set to a message on failure (may be null)
 * Returns
        > bool: true=open, false=failed
*/
bool SerialLink::open(const Config& cfg, QString* error){
    return openInternal(cfg, error, 3000);
}



/*******SerialLink::openInternal*******
 * Description
        > opens the serial port or the TCP connection described by cfg
 * Arguments
        > const Config& cfg: what to open and with which settings
        > QString* error: set to a message on failure (may be null)
        > int tcpTimeoutMs: how long to wait for a TCP connection (the GUI is blocked meanwhile)
 * Returns
        > bool: true=open, false=failed
*/
bool SerialLink::openInternal(const Config& cfg, QString* error, int tcpTimeoutMs){
    // close whatever was open before (without announcing it, the caller announces the new state)
        if (m_port.isOpen()){m_port.close();}
        if (m_tcpOpen || m_sock.state() != QAbstractSocket::UnconnectedState){m_tcpOpen = false; m_sock.abort();}

    m_cfg = cfg;
    m_rx.clear();

    if (cfg.tcp){
        m_sock.connectToHost(cfg.host, cfg.tcpPort);
        if (!m_sock.waitForConnected(tcpTimeoutMs)){
            if (error != nullptr){*error = m_sock.errorString();}
            m_sock.abort();
            return false;
        }
        m_tcpOpen = true;
    }
    else {
        m_port.setPortName(cfg.portName);

        if (!m_port.open(QIODevice::ReadWrite)){
            if (error != nullptr){*error = m_port.errorString();}
            return false;
        }

        applyPortSettings(cfg);
        m_port.clear();
    }

    m_wantOpen = true;

    emit connectionChanged(true);
    return true;
}



/*******SerialLink::device*******
 * Description
        > gets the I/O device in use (serial port or TCP socket)
 * Arguments
        > N/A
 * Returns
        > QIODevice*: the active device
*/
QIODevice* SerialLink::device(){
    if (m_cfg.tcp){return &m_sock;}
    return &m_port;
}



/*******SerialLink::onSocketDisconnected*******
 * Description
        > the TCP connection ended (SSH tunnel dropped, relay stopped): closes the link and, if enabled,
            reconnecting starts
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SerialLink::onSocketDisconnected(){
    if (!m_tcpOpen){return;}        // we closed it ourselves
    m_tcpOpen = false;

    failAllPending(QStringLiteral("disconnected"));
    emit errorMessage(QStringLiteral("Connection to %1 lost").arg(m_cfg.portName));
    emit connectionChanged(false);
}



/*******SerialLink::close*******
 * Description
        > closes the port because the user asked (disables auto reconnect until next open)
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SerialLink::close(){
    m_wantOpen = false;

    bool wasOpen = isOpen();
    if (m_port.isOpen()){m_port.close();}
    if (m_tcpOpen){m_tcpOpen = false; m_sock.abort();}

    if (wasOpen){
        failAllPending(QStringLiteral("disconnected"));
        emit connectionChanged(false);
    }
}



/*******SerialLink::isOpen*******
 * Description
        > tells whether the port is open
 * Arguments
        > N/A
 * Returns
        > bool: true=open
*/
bool SerialLink::isOpen() const {
    return m_port.isOpen() || m_tcpOpen;
}



/*******SerialLink::isReadOnly*******
 * Description
        > tells whether this connection is read-only (nothing is ever sent to the RP2350)
 * Arguments
        > N/A
 * Returns
        > bool: true=read-only
*/
bool SerialLink::isReadOnly() const {
    return m_cfg.readOnly;
}



/*******SerialLink::portName*******
 * Description
        > gets the name of the port in use (or last used)
 * Arguments
        > N/A
 * Returns
        > QString: port name such as "COM5" or "/dev/ttyACM0"
*/
QString SerialLink::portName() const {
    return m_cfg.portName;
}



/*******SerialLink::applyLiveConfig*******
 * Description
        > stores new settings and applies them now if the port is open
            (the port name is not changed while open)
 * Arguments
        > const Config& cfg: new settings
 * Returns
        > N/A
*/
void SerialLink::applyLiveConfig(const Config& cfg){
    // the connection target and read-only choice belong to the open connection, keep them
        Config keep = m_cfg;
        m_cfg = cfg;
        m_cfg.portName = keep.portName;
        m_cfg.tcp = keep.tcp;
        m_cfg.host = keep.host;
        m_cfg.tcpPort = keep.tcpPort;
        m_cfg.readOnly = keep.readOnly;

    if (!m_cfg.tcp){applyPortSettings(m_cfg);}
}



/*******SerialLink::setAutoReconnect*******
 * Description
        > turns automatic reconnecting on or off
 * Arguments
        > bool enable: true=reconnect when the RP2350 comes back
 * Returns
        > N/A
*/
void SerialLink::setAutoReconnect(bool enable){
    m_autoReconnect = enable;
}



/*******SerialLink::setCommandTimeoutMs*******
 * Description
        > sets how long to wait for a reply before giving up on a command
 * Arguments
        > int ms: timeout in milliseconds
 * Returns
        > N/A
*/
void SerialLink::setCommandTimeoutMs(int ms){
    m_timeoutMs = ms;
}



/*******SerialLink::sendCommand*******
 * Description
        > sends one command line (adds the line ending) and remembers who to
            tell when the reply arrives
 * Arguments
        > const QString& text: command without line ending
        > Source src: who sent it (Typed in terminal, Gui button, Background keep-alive)
        > Reply cb: called with the reply (optional)
        > bool frameReply: true for "status"/"telemetry", whose reply is an S/T frame (or ERR)
 * Returns
        > N/A
*/
void SerialLink::sendCommand(const QString& text, Source src, Reply cb, bool frameReply){
    if (!isOpen()){
        if (cb){cb(false, QStringLiteral("not connected"));}
        return;
    }

    // read-only connection: nothing may be sent
        if (m_cfg.readOnly){
            if (src == Source::Typed){emit errorMessage(QStringLiteral("Read-only connection: \"%1\" was not sent.").arg(text));}
            if (cb){cb(false, QStringLiteral("read-only connection"));}
            return;
        }

    Pending p;
    p.cmd = text;
    p.src = src;
    p.cb = cb;
    p.frameReply = frameReply;
    p.deadlineMs = QDateTime::currentMSecsSinceEpoch() + m_timeoutMs;
    m_pending.push_back(p);

    device()->write((text + m_cfg.eol).toUtf8());
    emit lineSent(text, (int)src);
}



/*******SerialLink::onReadyRead*******
 * Description
        > reads available bytes and handles every complete line
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SerialLink::onReadyRead(){
    m_rx.append(device()->readAll());

    int nl;
    while ((nl = m_rx.indexOf('\n')) >= 0){
        QByteArray raw = m_rx.left(nl);
        m_rx.remove(0, nl + 1);

        if (raw.endsWith('\r')){raw.chop(1);}
        handleLine(QString::fromUtf8(raw));
    }

    if (m_rx.size() > 65536){m_rx.clear();}     // no newline for ages: garbage, drop it
}



/*******SerialLink::handleLine*******
 * Description
        > routes a received line: final replies (OK / ERR, or S/T for frame
            commands) complete the oldest waiting command
 * Arguments
        > const QString& line: received line without line ending
 * Returns
        > N/A
*/
void SerialLink::handleLine(const QString& line){
    bool isOk = (line == QLatin1String("OK") || line.startsWith(QLatin1String("OK ")));
    bool isErr = (line == QLatin1String("ERR") || line.startsWith(QLatin1String("ERR ")));
    bool isFrame = (line.startsWith(QLatin1String("S,")) || line.startsWith(QLatin1String("T,")));

    if (isOk){completeFront(true, line.mid(3), line);}
    else if (isErr){completeFront(false, line.mid(4), line);}
    else if (isFrame && !m_pending.empty() && m_pending.front().frameReply){completeFront(true, line, line);}
    else {emit lineReceived(line, false);}
}



/*******SerialLink::completeFront*******
 * Description
        > finishes the oldest waiting command with a reply line
 * Arguments
        > bool ok: true for success
        > const QString& text: reply text for the callback
        > const QString& line: the full received line (for the terminal)
 * Returns
        > N/A
*/
void SerialLink::completeFront(bool ok, const QString& text, const QString& line){
    if (m_pending.empty()){
        emit lineReceived(line, false);
        return;
    }

    Pending p = m_pending.front();
    m_pending.pop_front();

    emit lineReceived(line, p.src == Source::Background);
    if (p.cb){p.cb(ok, text);}
}



/*******SerialLink::failAllPending*******
 * Description
        > ends every waiting command with a failure (port closed / lost)
 * Arguments
        > const QString& why: text passed to the callbacks
 * Returns
        > N/A
*/
void SerialLink::failAllPending(const QString& why){
    std::deque<Pending> old;
    old.swap(m_pending);

    for (const Pending& p : old){
        if (p.cb){p.cb(false, why);}
    }
}



/*******SerialLink::onPortError*******
 * Description
        > handles port errors; a vanished device closes the port and (if enabled)
            starts reconnecting
 * Arguments
        > QSerialPort::SerialPortError error: error code
 * Returns
        > N/A
*/
void SerialLink::onPortError(QSerialPort::SerialPortError error){
    if (error == QSerialPort::NoError){return;}

    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError || error == QSerialPort::PermissionError){
        QString msg = m_port.errorString();
        bool wasOpen = m_port.isOpen();
        m_port.close();
        failAllPending(QStringLiteral("disconnected"));

        emit errorMessage(QStringLiteral("Serial port error: %1").arg(msg));
        if (wasOpen){emit connectionChanged(false);}
    }
}



/*******SerialLink::onTick*******
 * Description
        > every 200 ms: times out unanswered commands and tries to reconnect
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SerialLink::onTick(){
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // time out old commands
        while (!m_pending.empty() && m_pending.front().deadlineMs < now){
            Pending p = m_pending.front();
            m_pending.pop_front();
            if (p.cb){p.cb(false, QStringLiteral("timeout"));}
        }

    // reconnect
        if (m_wantOpen && m_autoReconnect && !isOpen() && now - m_lastReconnectMs > 1000){
            m_lastReconnectMs = now;

            if (m_cfg.tcp){
                QString err;
                if (openInternal(m_cfg, &err, 400)){emit errorMessage(QStringLiteral("Reconnected to %1").arg(m_cfg.portName));}
                return;
            }

            const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
            for (const QSerialPortInfo& info : ports){
                if (info.portName() == m_cfg.portName){
                    QString err;
                    if (open(m_cfg, &err)){emit errorMessage(QStringLiteral("Reconnected to %1").arg(m_cfg.portName));}
                    break;
                }
            }
        }
}
