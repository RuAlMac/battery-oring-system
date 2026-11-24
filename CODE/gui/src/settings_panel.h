#ifndef SETTINGS_PANEL_H
#define SETTINGS_PANEL_H

/******************<Summary>*****************
 * Name: settings_panel.h
 * Purpose:
 *      > Shows every RP2350 setting (active value, editable pending value,
 *          default, limits) and the EEPROM buttons (Load / Save)
 *      > Edits change the RP2350's PENDING copy ("set"), Save writes the
 *          pending copy to the EEPROM and activates it ("save")
 * Written / updated by:
 *      > Claude
******************</Summary>*****************/

#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QTableWidget>
#include <QVector>
#include <QWidget>

#include "protocol.h"
#include "serial_link.h"

class SettingsPanel : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPanel(SerialLink* link, QWidget* parent = nullptr);

public slots:
    void refresh();
    void setConnected(bool connected);

signals:
    void activeValuesChanged(const QMap<QString, double>& values);

private slots:
    void onLine(const QString& line, bool background);
    void onLoadEeprom();
    void onSaveEeprom();
    void onDiscard();
    void onDefaults();
    void onItemChanged(QTableWidgetItem* item);

private:
    SerialLink* m_link;
    QTableWidget* m_table;
    QLabel* m_status;
    QLabel* m_unsavedLabel;
    QPushButton* m_loadBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_refreshBtn;
    QPushButton* m_discardBtn;
    QPushButton* m_defaultsBtn;

    QVector<proto::SettingRow> m_rows;
    QVector<proto::SettingRow> m_incoming;
    bool m_collecting = false;
    bool m_updating = false;

    void fillTable();
    void setStatus(const QString& text, bool error);
    void updateUnsavedLabel();
    void runThenRefresh(const QString& command, const QString& okText, const QString& errPrefix, bool popup);
};

#endif // SETTINGS_PANEL_H
