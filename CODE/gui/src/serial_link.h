#ifndef SERIAL_LINK_H
#define SERIAL_LINK_H

/******************<Summary>*****************
 * Name: serial_link.h
 * Purpose:
 *      > Owns the serial port, splits received data into lines, and matches
 *          "OK ..." / "ERR ..." reply lines to the commands that caused them
 *      > Every line sent or received is announced by a signal so the terminal
 *          view can show ALL traffic
 *      > Reconnects automatically (optional) when the RP2350 is unplugged and
 *          plugged back in
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

#include <deque>
#include <functional>

class SerialLink : public QObject {
    Q_OBJECT

public:
    enum class Source { Typed, Gui, Background };

    struct Config {
        QString portName;               // serial port name, or "tcp://host:port" for a network connection
        bool tcp = false;               // true = connect to host:tcpPort instead of a serial port
        QString host;
        quint16 tcpPort = 0;
        bool readOnly = false;          // true = never send anything to the RP2350, only listen
        qint32 baud = 115200;
        QSerialPort::DataBits dataBits = QSerialPort::Data8;
        QSerialPort::Parity parity = QSerialPort::NoParity;
        QSerialPort::StopBits stopBits = QSerialPort::OneStop;
        QSerialPort::FlowControl flow = QSerialPort::NoFlowControl;
        bool dtr = true;                // the RP2350 only streams while DTR is asserted
        bool rts = true;
        QString eol = QStringLiteral("\n");
    };

    // ok=true: text is what followed "OK"; ok=false: text follows "ERR" (or "timeout" / "not connected")
    using Reply = std::function<void(bool ok, const QString& text)>;

    explicit SerialLink(QObject* parent = nullptr);

    bool open(const Config& cfg, QString* error);
    void close();
    bool isOpen() const;
    bool isReadOnly() const;
    QString portName() const;
    void applyLiveConfig(const Config& cfg);
    void setAutoReconnect(bool enable);
    void setCommandTimeoutMs(int ms);
    void sendCommand(const QString& text, Source src = Source::Gui, Reply cb = {}, bool frameReply = false);

signals:
    void connectionChanged(bool open);
    void lineSent(const QString& line, int source);
    void lineReceived(const QString& line, bool background);
    void errorMessage(const QString& message);

private slots:
    void onReadyRead();
    void onPortError(QSerialPort::SerialPortError error);
    void onTick();
    void onSocketDisconnected();

private:
    struct Pending {
        QString cmd;
        Source src;
        Reply cb;
        bool frameReply;
        qint64 deadlineMs;
    };

    QSerialPort m_port;
    QTcpSocket m_sock;
    bool m_tcpOpen = false;         // true while a TCP connection is (supposed to be) up
    Config m_cfg;
    QByteArray m_rx;
    std::deque<Pending> m_pending;
    QTimer m_tick;
    bool m_wantOpen = false;        // user wants the port open (so reconnect if it drops)
    bool m_autoReconnect = true;
    int m_timeoutMs = 3000;
    qint64 m_lastReconnectMs = 0;

    QIODevice* device();
    bool openInternal(const Config& cfg, QString* error, int tcpTimeoutMs);
    void applyPortSettings(const Config& cfg);
    void handleLine(const QString& line);
    void failAllPending(const QString& why);
    void completeFront(bool ok, const QString& text, const QString& line);
};

#endif // SERIAL_LINK_H
