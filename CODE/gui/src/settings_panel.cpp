#include "settings_panel.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {
enum Column {ColName = 0, ColUnits, ColActive, ColPending, ColDefault, ColMin, ColMax, ColCount};
}

/*******SettingsPanel::SettingsPanel*******
 * Description
        > builds the settings table and the EEPROM / edit buttons
 * Arguments
        > SerialLink* link: serial link used to talk to the RP2350
        > QWidget* parent: Qt parent
 * Returns
        > N/A
*/
SettingsPanel::SettingsPanel(SerialLink* link, QWidget* parent) : QWidget(parent), m_link(link) {
    auto* root = new QVBoxLayout(this);

    // EEPROM group
        auto* eeprom = new QGroupBox(tr("Onboard EEPROM"), this);
        auto* eepromLayout = new QHBoxLayout(eeprom);

        m_loadBtn = new QPushButton(tr("Load settings from EEPROM"), eeprom);
        m_saveBtn = new QPushButton(tr("Save settings to EEPROM"), eeprom);
        m_loadBtn->setMinimumHeight(40);
        m_saveBtn->setMinimumHeight(40);
        m_loadBtn->setToolTip(tr("Re-read the EEPROM. Replaces both the active and the pending values on the RP2350."));
        m_saveBtn->setToolTip(tr("Check the pending values, write them to the EEPROM, verify, and make them active."));
        m_saveBtn->setStyleSheet(QStringLiteral("QPushButton {font-weight: bold;}"));

        m_unsavedLabel = new QLabel(eeprom);

        eepromLayout->addWidget(m_loadBtn);
        eepromLayout->addWidget(m_saveBtn);
        eepromLayout->addSpacing(16);
        eepromLayout->addWidget(m_unsavedLabel, 1);
        root->addWidget(eeprom);

    // table
        m_table = new QTableWidget(0, ColCount, this);
        m_table->setHorizontalHeaderLabels({tr("Setting"), tr("Units"), tr("Active (in use)"), tr("Pending (edit me)"),
                                            tr("Default"), tr("Min"), tr("Max")});
        m_table->verticalHeader()->setVisible(false);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(ColUnits, QHeaderView::ResizeToContents);
        root->addWidget(m_table, 1);

    // edit buttons
        auto* row = new QHBoxLayout();
        m_refreshBtn = new QPushButton(tr("Refresh table"), this);
        m_discardBtn = new QPushButton(tr("Discard unsaved edits"), this);
        m_defaultsBtn = new QPushButton(tr("Put defaults into pending"), this);
        m_discardBtn->setToolTip(tr("Set pending = active on the RP2350."));
        m_defaultsBtn->setToolTip(tr("Fill the pending column with the default values. Nothing is stored until you save."));
        row->addWidget(m_refreshBtn);
        row->addWidget(m_discardBtn);
        row->addWidget(m_defaultsBtn);
        row->addStretch(1);
        root->addLayout(row);

        m_status = new QLabel(this);
        m_status->setWordWrap(true);
        root->addWidget(m_status);

    connect(m_loadBtn, &QPushButton::clicked, this, &SettingsPanel::onLoadEeprom);
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsPanel::onSaveEeprom);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SettingsPanel::refresh);
    connect(m_discardBtn, &QPushButton::clicked, this, &SettingsPanel::onDiscard);
    connect(m_defaultsBtn, &QPushButton::clicked, this, &SettingsPanel::onDefaults);
    connect(m_table, &QTableWidget::itemChanged, this, &SettingsPanel::onItemChanged);
    connect(m_link, &SerialLink::lineReceived, this, &SettingsPanel::onLine);

    setConnected(false);
    setStatus(tr("Connect to the RP2350 to see its settings."), false);
}



/*******SettingsPanel::setConnected*******
 * Description
        > enables or disables the buttons depending on the connection
 * Arguments
        > bool connected: true=port open
 * Returns
        > N/A
*/
void SettingsPanel::setConnected(bool connected){
    m_loadBtn->setEnabled(connected);
    m_saveBtn->setEnabled(connected);
    m_refreshBtn->setEnabled(connected);
    m_discardBtn->setEnabled(connected);
    m_defaultsBtn->setEnabled(connected);
    m_table->setEnabled(connected);

    if (!connected){
        m_unsavedLabel->clear();
        setStatus(tr("Not connected."), false);
    }
}



/*******SettingsPanel::setStatus*******
 * Description
        > shows a message under the table
 * Arguments
        > const QString& text: message
        > bool error: true=red text
 * Returns
        > N/A
*/
void SettingsPanel::setStatus(const QString& text, bool error){
    m_status->setText(text);
    m_status->setStyleSheet(error ? QStringLiteral("color: #d32f2f; font-weight: bold;") : QString());
}



/*******SettingsPanel::onLine*******
 * Description
        > collects SET lines while a "list" is in progress
 * Arguments
        > const QString& line: received line
        > bool background: unused
 * Returns
        > N/A
*/
void SettingsPanel::onLine(const QString& line, bool background){
    Q_UNUSED(background);
    if (!m_collecting){return;}

    proto::SettingRow row;
    if (proto::parseSettingLine(line, &row)){m_incoming.push_back(row);}
}



/*******SettingsPanel::refresh*******
 * Description
        > asks the RP2350 for all settings ("list") and refills the table
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::refresh(){
    if (!m_link->isOpen()){return;}

    m_incoming.clear();
    m_collecting = true;

    m_link->sendCommand(QStringLiteral("list"), SerialLink::Source::Gui, [this](bool ok, const QString& text){
        m_collecting = false;

        if (!ok){
            setStatus(tr("Could not read settings: %1").arg(text), true);
            return;
        }

        m_rows = m_incoming;
        fillTable();
    });
}



/*******SettingsPanel::fillTable*******
 * Description
        > writes m_rows into the table and highlights rows with unsaved edits
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::fillTable(){
    m_updating = true;
    m_table->setRowCount(m_rows.size());

    auto makeItem = [](const QString& text, bool editable) {
        auto* item = new QTableWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (editable){flags |= Qt::ItemIsEditable;}
        item->setFlags(flags);
        return item;
    };

    for (int r = 0; r < m_rows.size(); r++){
        const proto::SettingRow& s = m_rows[r];
        bool unsaved = (s.pending != s.active);

        QTableWidgetItem* items[ColCount];
        items[ColName] = makeItem(s.name, false);
        items[ColUnits] = makeItem(s.units, false);
        items[ColActive] = makeItem(proto::formatSettingValue(s.active, s.units), false);
        items[ColPending] = makeItem(proto::formatSettingValue(s.pending, s.units), true);
        items[ColDefault] = makeItem(proto::formatSettingValue(s.def, s.units), false);
        items[ColMin] = makeItem(proto::formatSettingValue(s.min, s.units), false);
        items[ColMax] = makeItem(proto::formatSettingValue(s.max, s.units), false);

        items[ColName]->setToolTip(tr("%1 (%2 to %3 %4)").arg(s.name, proto::formatSettingValue(s.min, s.units),
                                                              proto::formatSettingValue(s.max, s.units), s.units));

        for (int c = 0; c < ColCount; c++){
            // the editable column has its own color; unsaved rows turn amber
            QColor bg;
            if (c == ColPending){bg = unsaved ? QColor(255, 202, 40) : QColor(210, 232, 252);}
            else if (unsaved){bg = QColor(255, 236, 179);}

            if (bg.isValid()){
                items[c]->setBackground(QBrush(bg));
                items[c]->setForeground(QBrush(QColor(30, 30, 30)));
            }
            m_table->setItem(r, c, items[c]);
        }

        QFont f = items[ColPending]->font();
        f.setBold(true);
        items[ColPending]->setFont(f);
    }

    m_updating = false;
    updateUnsavedLabel();

    QMap<QString, double> active;
    for (const proto::SettingRow& s : m_rows){active.insert(s.name, s.active);}
    emit activeValuesChanged(active);
}



/*******SettingsPanel::updateUnsavedLabel*******
 * Description
        > updates the "N unsaved changes" text next to the EEPROM buttons
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::updateUnsavedLabel(){
    int count = 0;
    for (const proto::SettingRow& s : m_rows){
        if (s.pending != s.active){count++;}
    }

    if (count == 0){
        m_unsavedLabel->setText(tr("No unsaved changes."));
        m_unsavedLabel->setStyleSheet(QStringLiteral("color: #2e7d32;"));
    }
    else {
        m_unsavedLabel->setText(tr("%n unsaved change(s) - press \"Save settings to EEPROM\" to keep them.", nullptr, count));
        m_unsavedLabel->setStyleSheet(QStringLiteral("color: #b26a00; font-weight: bold;"));
    }
}



/*******SettingsPanel::onItemChanged*******
 * Description
        > the user edited a pending value: send "set <name> <value>"
 * Arguments
        > QTableWidgetItem* item: edited cell
 * Returns
        > N/A
*/
void SettingsPanel::onItemChanged(QTableWidgetItem* item){
    if (m_updating || item->column() != ColPending){return;}

    int row = item->row();
    if (row < 0 || row >= m_rows.size()){return;}

    bool ok = false;
    const QString typed = item->text().trimmed().toLower();
    double value = typed.toDouble(&ok);
    if (!ok && m_rows[row].units == QLatin1String("bool")){       // boolean settings also accept words
        if (typed == QLatin1String("on") || typed == QLatin1String("true") || typed == QLatin1String("yes")){value = 1.0; ok = true;}
        else if (typed == QLatin1String("off") || typed == QLatin1String("false") || typed == QLatin1String("no")){value = 0.0; ok = true;}
    }
    if (!ok){
        setStatus(tr("\"%1\" is not a number.").arg(item->text()), true);
        fillTable();
        return;
    }

    QString name = m_rows[row].name;
    QString cmd = QStringLiteral("set %1 %2").arg(name, QString::number(value, 'g', 9));

    m_link->sendCommand(cmd, SerialLink::Source::Gui, [this, name](bool good, const QString& text){
        if (good){
            const QMap<QString, QString> kv = proto::parseKeyValues(text);
            setStatus(tr("%1 pending value set to %2 (not saved yet).").arg(name, kv.value(QStringLiteral("pending"))), false);
        }
        else {setStatus(tr("Rejected: %1").arg(text), true);}

        refresh();      // always re-sync with what the RP2350 really holds
    });
}



/*******SettingsPanel::runThenRefresh*******
 * Description
        > sends a command, reports the result, then refreshes the table
 * Arguments
        > const QString& command: command to send
        > const QString& okText: status text on success
        > const QString& errPrefix: text shown before the RP2350's error message
        > bool popup: true=also show failures in a message box
 * Returns
        > N/A
*/
void SettingsPanel::runThenRefresh(const QString& command, const QString& okText, const QString& errPrefix, bool popup){
    m_link->sendCommand(command, SerialLink::Source::Gui, [this, okText, errPrefix, popup](bool ok, const QString& text){
        if (ok){setStatus(okText, false);}
        else {
            setStatus(errPrefix + text, true);
            if (popup){QMessageBox::warning(this, tr("RP2350"), errPrefix + text);}
        }

        refresh();
    });
}



/*******SettingsPanel::onLoadEeprom*******
 * Description
        > "Load settings from EEPROM" button
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::onLoadEeprom(){
    runThenRefresh(QStringLiteral("reload"), tr("Loaded settings from the EEPROM."), tr("Load failed: "), true);
}



/*******SettingsPanel::onSaveEeprom*******
 * Description
        > "Save settings to EEPROM" button
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::onSaveEeprom(){
    runThenRefresh(QStringLiteral("save"), tr("Saved to the EEPROM and now active."), tr("Save failed: "), true);
}



/*******SettingsPanel::onDiscard*******
 * Description
        > "Discard unsaved edits" button
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::onDiscard(){
    runThenRefresh(QStringLiteral("discard"), tr("Unsaved edits discarded."), tr("Discard failed: "), false);
}



/*******SettingsPanel::onDefaults*******
 * Description
        > "Put defaults into pending" button
 * Arguments
        > N/A
 * Returns
        > N/A
*/
void SettingsPanel::onDefaults(){
    runThenRefresh(QStringLiteral("defaults"), tr("Default values placed in the pending column. Save to keep them."), tr("Failed: "), false);
}
