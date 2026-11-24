#include "terminal_panel.h"

#include <QApplication>
#include <QCompleter>
#include <QDateTime>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QApplication>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

/*******TerminalPanel::TerminalPanel*******
 * Description
        > builds the terminal view, filter check boxes and command line
 * Arguments
        > SerialLink* link: serial link used to send typed commands
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
TerminalPanel::TerminalPanel(SerialLink* link, QWidget* parent) : QWidget(parent), m_link(link) {
    auto* root = new QVBoxLayout(this);

    // toolbar row
        auto* bar = new QHBoxLayout();
        m_pauseBtn = new QPushButton(tr("Pause"), this);
        m_pauseBtn->setCheckable(true);
        m_pauseBtn->setToolTip(tr("Freeze the view so you can read it. New lines are kept and appear when you resume."));
        auto* clearBtn = new QPushButton(tr("Clear"), this);
        auto* saveBtn = new QPushButton(tr("Save log..."), this);

        m_showFrames = new QCheckBox(tr("Show S/T data frames"), this);
        m_showFrames->setChecked(true);
        m_showFrames->setToolTip(tr("Untick to hide the streaming battery frames and keep only commands and replies."));
        m_showBackground = new QCheckBox(tr("Show keep-alive pings"), this);
        m_timestamps = new QCheckBox(tr("Timestamps"), this);
        m_timestamps->setChecked(true);
        m_autoscroll = new QCheckBox(tr("Auto-scroll"), this);
        m_autoscroll->setChecked(true);

        bar->addWidget(m_pauseBtn);
        bar->addWidget(clearBtn);
        bar->addWidget(saveBtn);
        bar->addSpacing(12);
        bar->addWidget(m_showFrames);
        bar->addWidget(m_showBackground);
        bar->addWidget(m_timestamps);
        bar->addWidget(m_autoscroll);
        bar->addStretch(1);
        root->addLayout(bar);

    // terminal view
        m_view = new QPlainTextEdit(this);
        m_view->setReadOnly(true);
        m_view->setMaximumBlockCount(5000);
        m_view->setLineWrapMode(QPlainTextEdit::NoWrap);
        m_view->setFont(QApplication::font());
        m_view->setStyleSheet(QStringLiteral("QPlainTextEdit {background-color: #1e1e1e; color: #d4d4d4;}"));
        root->addWidget(m_view, 1);

    // command line
        auto* inputRow = new QHBoxLayout();
        m_input = new QLineEdit(this);
        m_input->setFont(QApplication::font());
        m_input->setPlaceholderText(tr("Type a command and press Enter (try: help). Up / Down = history."));
        m_input->installEventFilter(this);

        auto* completer = new QCompleter(QStringList{
            "help", "ping", "ver", "info", "mode normal", "mode debug", "mode silent", "status", "telemetry",
            "list", "get ", "set ", "validate", "save", "discard", "defaults", "reload", "rate ", "stream ",
            "ovr", "ovr cell ", "ovr batt ", "ovr temp ", "ovr clear"}, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_input->setCompleter(completer);

        m_sendBtn = new QPushButton(tr("Send"), this);
        inputRow->addWidget(m_input, 1);
        inputRow->addWidget(m_sendBtn);
        root->addLayout(inputRow);

    connect(m_input, &QLineEdit::returnPressed, this, &TerminalPanel::onSendClicked);
    connect(m_sendBtn, &QPushButton::clicked, this, &TerminalPanel::onSendClicked);
    connect(m_pauseBtn, &QPushButton::toggled, this, &TerminalPanel::onPauseToggled);
    connect(clearBtn, &QPushButton::clicked, m_view, &QPlainTextEdit::clear);
    connect(saveBtn, &QPushButton::clicked, this, &TerminalPanel::onSaveLog);

    connect(m_link, &SerialLink::lineSent, this, &TerminalPanel::onLineSent);
    connect(m_link, &SerialLink::lineReceived, this, &TerminalPanel::onLineReceived);
}



/*******TerminalPanel::colorForReceived*******
 * Description
        > picks the text color for a received line by its type
 * Arguments
        > const QString& line: received line
 * Returns
        > QString: color name / hex
*/
QString TerminalPanel::colorForReceived(const QString& line){
    if (line.startsWith(QLatin1String("S,")) || line.startsWith(QLatin1String("T,"))){return QStringLiteral("#808080");}
    if (line.startsWith(QLatin1String("OK"))){return QStringLiteral("#6fbf73");}
    if (line.startsWith(QLatin1String("ERR"))){return QStringLiteral("#f44747");}
    if (line.startsWith(QLatin1String("EVT"))){return QStringLiteral("#e5a50a");}
    if (line.startsWith(QLatin1String("#"))){return QStringLiteral("#9e9e9e");}
    if (line.startsWith(QLatin1String("SET "))){return QStringLiteral("#c5c5e0");}
    return QStringLiteral("#d4d4d4");
}



/*******TerminalPanel::writeEntry*******
 * Description
        > appends one colored line to the view (optionally with a timestamp)
 * Arguments
        > const Entry& entry: text and color
 * Returns
        > N/A
*/
void TerminalPanel::writeEntry(const Entry& entry){
    QScrollBar* bar = m_view->verticalScrollBar();
    int oldValue = bar->value();

    QTextCharFormat fmt;
    fmt.setForeground(QColor(entry.color));

    QTextCursor cursor(m_view->document());
    cursor.movePosition(QTextCursor::End);
    if (!m_view->document()->isEmpty()){cursor.insertBlock();}

    QString text = entry.text;
    if (m_timestamps->isChecked()){text = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz  ")) + text;}
    cursor.insertText(text, fmt);

    if (m_autoscroll->isChecked()){bar->setValue(bar->maximum());}
    else {bar->setValue(oldValue);}
}



/*******TerminalPanel::addEntry*******
 * Description
        > shows a line now, or keeps it for later while paused
 * Arguments
        > const QString& text: line text
        > const QString& color: text color
 * Returns
        > N/A
*/
void TerminalPanel::addEntry(const QString& text, const QString& color){
    Entry e{text, color};

    if (m_paused){
        if (m_held.size() < 20000){m_held.push_back(e);}
        return;
    }

    writeEntry(e);
}



/*******TerminalPanel::onLineSent*******
 * Description
        > shows a line that was sent to the RP2350
 * Arguments
        > const QString& line: command text
        > int source: SerialLink::Source (typed / GUI / background)
 * Returns
        > N/A
*/
void TerminalPanel::onLineSent(const QString& line, int source){
    auto src = (SerialLink::Source)source;

    if (src == SerialLink::Source::Background){
        if (!m_showBackground->isChecked()){return;}
        addEntry(QStringLiteral("[auto] > ") + line, QStringLiteral("#7a8fa6"));
    }
    else if (src == SerialLink::Source::Gui){addEntry(QStringLiteral("[gui]  > ") + line, QStringLiteral("#7fb2e5"));}
    else {addEntry(QStringLiteral("> ") + line, QStringLiteral("#4fc1ff"));}
}



/*******TerminalPanel::onLineReceived*******
 * Description
        > shows a line received from the RP2350 (subject to the filters)
 * Arguments
        > const QString& line: received line
        > bool background: true if it is the reply to a keep-alive command
 * Returns
        > N/A
*/
void TerminalPanel::onLineReceived(const QString& line, bool background){
    if (background && !m_showBackground->isChecked()){return;}

    bool isFrame = line.startsWith(QLatin1String("S,")) || line.startsWith(QLatin1String("T,"));
    if (isFrame && !m_showFrames->isChecked()){return;}

    addEntry(line, colorForReceived(line));
}



/*******TerminalPanel::appendNote*******
 * Description
        > shows a message from the GUI itself (connected, disconnected, errors)
 * Arguments
        > const QString& text: message
 * Returns
        > N/A
*/
void TerminalPanel::appendNote(const QString& text){
    addEntry(QStringLiteral("-- ") + text + QStringLiteral(" --"), QStringLiteral("#c586c0"));
}



/*******TerminalPanel::onSendClicked*******
 * Description
        > sends the text typed in the command line
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void TerminalPanel::onSendClicked(){
    QString text = m_input->text().trimmed();
    if (text.isEmpty()){return;}

    m_input->clear();

    if (m_history.isEmpty() || m_history.last() != text){m_history.push_back(text);}
    m_historyPos = m_history.size();

    if (!m_link->isOpen()){
        appendNote(tr("not connected - command not sent: %1").arg(text));
        return;
    }

    QString first = text.section(' ', 0, 0).toLower();
    bool frameReply = (first == QLatin1String("status") || first == QLatin1String("telemetry"));

    m_link->sendCommand(text, SerialLink::Source::Typed, [this, first](bool ok, const QString& reply){
        Q_UNUSED(ok);
        Q_UNUSED(reply);
        emit typedCommandFinished(first);
    }, frameReply);
}



/*******TerminalPanel::onPauseToggled*******
 * Description
        > pauses or resumes the view
 * Arguments
        > bool paused: true=pause
 * Returns
        > N/A
*/
void TerminalPanel::onPauseToggled(bool paused){
    m_paused = paused;
    m_pauseBtn->setText(paused ? tr("Resume") : tr("Pause"));

    if (!paused){
        const QVector<Entry> held = m_held;
        m_held.clear();
        for (const Entry& e : held){writeEntry(e);}
    }
}



/*******TerminalPanel::onSaveLog*******
 * Description
        > saves the terminal text to a file chosen by the user
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void TerminalPanel::onSaveLog(){
    QString name = QFileDialog::getSaveFileName(this, tr("Save terminal log"),
        QDateTime::currentDateTime().toString(QStringLiteral("'battery_log_'yyyyMMdd_HHmmss'.txt'")),
        tr("Text files (*.txt);;All files (*)"));
    if (name.isEmpty()){return;}

    QFile file(name);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QMessageBox::warning(this, tr("Save log"), tr("Could not write %1").arg(name));
        return;
    }

    file.write(m_view->toPlainText().toUtf8());
}



/*******TerminalPanel::eventFilter*******
 * Description
        > gives the command line shell-style history on Up / Down
 * Arguments
        > QObject* watched: object that got the event
        > QEvent* event: the event
 * Returns
        > bool: true if the event was handled
*/
bool TerminalPanel::eventFilter(QObject* watched, QEvent* event){
    if (watched == m_input && event->type() == QEvent::KeyPress){
        auto* key = static_cast<QKeyEvent*>(event);

        if (key->key() == Qt::Key_Up && !m_history.isEmpty()){
            if (m_historyPos > 0){m_historyPos--;}
            m_input->setText(m_history.at(m_historyPos));
            return true;
        }
        if (key->key() == Qt::Key_Down && !m_history.isEmpty()){
            if (m_historyPos < m_history.size() - 1){
                m_historyPos++;
                m_input->setText(m_history.at(m_historyPos));
            }
            else {
                m_historyPos = m_history.size();
                m_input->clear();
            }
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}
