#ifndef TERMINAL_PANEL_H
#define TERMINAL_PANEL_H

/******************<Summary>*****************
 * Name: terminal_panel.h
 * Purpose:
 *      > Terminal-like view of EVERYTHING sent to and received from the RP2350,
 *          with a command line at the bottom for typing commands directly
 *      > Filters (hide telemetry frames, hide background traffic), pause,
 *          clear, and save-to-file keep a busy stream readable
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QCheckBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "serial_link.h"

class TerminalPanel : public QWidget {
    Q_OBJECT

public:
    explicit TerminalPanel(SerialLink* link, QWidget* parent = nullptr);

public slots:
    void onLineSent(const QString& line, int source);
    void onLineReceived(const QString& line, bool background);
    void appendNote(const QString& text);

signals:
    void typedCommandFinished(const QString& firstWord);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSendClicked();
    void onPauseToggled(bool paused);
    void onSaveLog();

private:
    struct Entry {
        QString text;
        QString color;
    };

    SerialLink* m_link;
    QPlainTextEdit* m_view;
    QLineEdit* m_input;
    QPushButton* m_sendBtn;
    QPushButton* m_pauseBtn;
    QCheckBox* m_showFrames;
    QCheckBox* m_showBackground;
    QCheckBox* m_timestamps;
    QCheckBox* m_autoscroll;

    QVector<Entry> m_held;          // lines received while paused
    QStringList m_history;
    int m_historyPos = 0;
    bool m_paused = false;

    void addEntry(const QString& text, const QString& color);
    void writeEntry(const Entry& entry);
    static QString colorForReceived(const QString& line);
};

#endif // TERMINAL_PANEL_H
